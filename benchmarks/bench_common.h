// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 四个 benchmark 共用的计时、统计与命令行小工具。
// 只依赖标准库和平台进程号接口，不引入 Valley 私有头。

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace bench {

inline std::uint64_t now_ns() noexcept
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

inline void cpu_relax() noexcept
{
#if defined(_WIN32)
    YieldProcessor();
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    asm volatile("yield" ::: "memory");
#else
    std::this_thread::yield();
#endif
}

inline std::string process_tag(const char* prefix)
{
#if defined(_WIN32)
    const auto pid = static_cast<unsigned long>(::GetCurrentProcessId());
#else
    const auto pid = static_cast<unsigned long>(::getpid());
#endif
    return std::string{prefix} + "_" + std::to_string(pid);
}

inline std::size_t default_threads()
{
    const auto hardware = std::thread::hardware_concurrency();
    if (hardware == 0)
        return 4;
    return std::min<std::size_t>(hardware, 4);
}

inline std::size_t parse_size(const char* text, const char* option,
                              bool allow_zero = false)
{
    if (text == nullptr || text[0] == '\0' || text[0] == '-') {
        std::fprintf(stderr, "%s: invalid value: %s\n", option,
                     text == nullptr ? "" : text);
        std::exit(2);
    }

    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long long>(
                    (std::numeric_limits<std::size_t>::max)()) ||
        (!allow_zero && value == 0)) {
        std::fprintf(stderr, "%s: invalid value: %s\n", option, text);
        std::exit(2);
    }
    return static_cast<std::size_t>(value);
}

struct Summary
{
    std::size_t count = 0;
    double mean = 0;
    double min = 0;
    double p50 = 0;
    double p90 = 0;
    double p99 = 0;
    double p999 = 0;
    double max = 0;
};

inline Summary summarize(std::vector<std::uint64_t> samples)
{
    Summary result;
    if (samples.empty())
        return result;

    std::sort(samples.begin(), samples.end());
    const auto pick = [&samples](double quantile) {
        const auto index = static_cast<std::size_t>(
            quantile * static_cast<double>(samples.size() - 1));
        return static_cast<double>(samples[index]);
    };

    double sum = 0;
    for (const auto value : samples)
        sum += static_cast<double>(value);

    result.count = samples.size();
    result.mean = sum / static_cast<double>(samples.size());
    result.min = static_cast<double>(samples.front());
    result.p50 = pick(0.50);
    result.p90 = pick(0.90);
    result.p99 = pick(0.99);
    result.p999 = pick(0.999);
    result.max = static_cast<double>(samples.back());
    return result;
}

inline void print_latency_ns(const char* tag, const Summary& summary)
{
    std::printf(
        "%-18s n=%-7zu mean=%8.1f p50=%8.1f p90=%8.1f p99=%8.1f "
        "p99.9=%8.1f max=%8.1f ns\n",
        tag, summary.count, summary.mean, summary.p50, summary.p90,
        summary.p99, summary.p999, summary.max);
}

inline void print_latency_us(const char* tag, const Summary& summary)
{
    std::printf(
        "%-18s n=%-7zu mean=%8.2f p50=%8.2f p90=%8.2f p99=%8.2f "
        "p99.9=%8.2f max=%8.2f us\n",
        tag, summary.count, summary.mean / 1000.0, summary.p50 / 1000.0,
        summary.p90 / 1000.0, summary.p99 / 1000.0, summary.p999 / 1000.0,
        summary.max / 1000.0);
}

}  // namespace bench
