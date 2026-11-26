// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

// ipc::Event 通知 benchmark：
//   1. emit-only 吞吐：只置位/发信号，不等待；
//   2. 同线程 emit + wait：纯 API 路径成本；
//   3. 跨线程 emit -> wait：包含一次线程唤醒。
//
//   event_bench --samples 20000 --emit-samples 2000000
//   event_bench --no-cross
//
// 事件通过公开的 ipc/event.h + ipc/notification.h 创建，region 是本地对齐内存。

#include "ipc/event.h"
#include "ipc/notification.h"

#include "bench_common.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options
{
    std::size_t samples = 20000;
    std::size_t emit_samples = 2000000;
    std::size_t warmup = 2000;
    bool cross_thread = true;
};

void print_usage(const char* exe)
{
    std::printf(
        "usage: %s [options]\n"
        "  --samples N       latency samples, default 20000\n"
        "  --emit-samples N  emit-only iterations, default 2000000\n"
        "  --warmup N        warmup iterations, default 2000\n"
        "  --no-cross        skip cross-thread wakeup measurement\n"
        "  -h, --help        show this help\n",
        exe);
}

Options parse_args(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s: missing value for %s\n", argv[0],
                             name);
                std::exit(2);
            }
            return argv[++i];
        };

        if (arg == "--samples")
            options.samples = bench::parse_size(value("--samples"), "--samples");
        else if (arg == "--emit-samples")
            options.emit_samples =
                bench::parse_size(value("--emit-samples"), "--emit-samples");
        else if (arg == "--warmup")
            options.warmup =
                bench::parse_size(value("--warmup"), "--warmup", true);
        else if (arg == "--no-cross")
            options.cross_thread = false;
        else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            std::fprintf(stderr, "%s: unknown argument: %s\n", argv[0],
                         arg.c_str());
            print_usage(argv[0]);
            std::exit(2);
        }
    }

    if (options.samples > 100'000'000 || options.emit_samples > 100'000'000 ||
        options.warmup > 100'000'000) {
        std::fprintf(stderr, "event_bench: sample count is too large\n");
        std::exit(2);
    }
    return options;
}

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4324)  // 有意按 cache line 对齐，允许尾部填充
#endif
struct Shared_state
{
    alignas(64) std::atomic<std::uint64_t> armed{0};
    alignas(64) std::atomic<std::uint64_t> start{0};
    alignas(64) std::atomic<std::uint64_t> done{0};
    alignas(64) std::atomic<bool> ready{false};
    alignas(64) std::atomic<bool> abort{false};
};
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

int run(const Options& options)
{
    constexpr std::size_t STORAGE_SIZE = 4096;
    if (ipc::Notification::required_size() > STORAGE_SIZE) {
        std::fprintf(stderr, "event_bench: notification storage is too small\n");
        return 1;
    }

    alignas(64) std::byte storage[STORAGE_SIZE]{};
    const std::string group = bench::process_tag("valley_event_bench");

    auto notification = ipc::Notification::create(
        storage, sizeof storage, group, {"tick"});
    if (!notification.valid()) {
        std::fprintf(stderr, "event_bench: notification create failed\n");
        return 1;
    }
    if (!ipc::Event::bind(group, notification)) {
        std::fprintf(stderr, "event_bench: event bind failed\n");
        return 1;
    }

    {
        ipc::Event event{group, "tick"};
        if (!event.valid()) {
            std::fprintf(stderr, "event_bench: event handle is invalid\n");
            ipc::Event::unbind(group);
            return 1;
        }

        // -----------------------------------------------------------------
        // emit-only
        // -----------------------------------------------------------------
        notification.reset();
        for (std::size_t i = 0; i < options.warmup; ++i)
            event.emit();
        notification.reset();

        const auto emit_begin = bench::now_ns();
        for (std::size_t i = 0; i < options.emit_samples; ++i)
            event.emit();
        const auto emit_elapsed = bench::now_ns() - emit_begin;
        notification.reset();

        const double emit_ns =
            static_cast<double>(emit_elapsed) /
            static_cast<double>(options.emit_samples);
        const double emit_per_second =
            static_cast<double>(options.emit_samples) /
            (static_cast<double>(emit_elapsed) / 1'000'000'000.0);
        std::printf(
            "event_bench: emit-only iterations=%zu ns/op=%.1f Mops/s=%.2f\n",
            options.emit_samples, emit_ns, emit_per_second / 1'000'000.0);

        // -----------------------------------------------------------------
        // 同一线程 emit + wait
        // -----------------------------------------------------------------
        std::size_t self_failures = 0;
        for (std::size_t i = 0; i < options.warmup; ++i) {
            event.emit();
            if (!notification.wait(std::chrono::milliseconds{0}) ||
                notification.poll() != 0) {
                ++self_failures;
                notification.reset();
            }
        }
        self_failures = 0;

        std::vector<std::uint64_t> self_samples;
        self_samples.reserve(options.samples);
        for (std::size_t i = 0; i < options.samples; ++i) {
            const auto begin = bench::now_ns();
            event.emit();
            if (!notification.wait(std::chrono::milliseconds{0}) ||
                notification.poll() != 0) {
                ++self_failures;
                notification.reset();
                continue;
            }
            self_samples.push_back(bench::now_ns() - begin);
        }

        bench::print_latency_us("event/self", bench::summarize(self_samples));
        if (self_failures != 0) {
            std::fprintf(stderr,
                         "event_bench: same-thread wait failed %zu times\n",
                         self_failures);
            ipc::Event::unbind(group);
            return 3;
        }

        // -----------------------------------------------------------------
        // 跨线程 emit -> wait
        // -----------------------------------------------------------------
        if (options.cross_thread) {
            Shared_state shared;
            const std::uint64_t total =
                static_cast<std::uint64_t>(options.warmup) +
                static_cast<std::uint64_t>(options.samples);
            std::vector<std::uint64_t> cross_samples;
            cross_samples.reserve(options.samples);

            std::thread consumer([&] {
                shared.ready.store(true, std::memory_order_release);
                for (std::uint64_t sequence = 1; sequence <= total;
                     ++sequence) {
                    // 先把“已就绪”发布给 producer，再进入 wait；producer
                    // 看到 armed 后才 emit，尽量测到真正阻塞后的唤醒。
                    shared.armed.store(sequence, std::memory_order_release);
                    if (!notification.wait(std::chrono::milliseconds{1000}) ||
                        notification.poll() != 0) {
                        shared.abort.store(true, std::memory_order_release);
                        shared.done.store(sequence,
                                          std::memory_order_release);
                        return;
                    }

                    const auto end = bench::now_ns();
                    const auto begin =
                        shared.start.load(std::memory_order_acquire);
                    if (sequence > options.warmup)
                        cross_samples.push_back(end - begin);
                    shared.done.store(sequence, std::memory_order_release);
                }
            });

            while (!shared.ready.load(std::memory_order_acquire))
                bench::cpu_relax();

            for (std::uint64_t sequence = 1; sequence <= total; ++sequence) {
                if (shared.abort.load(std::memory_order_acquire))
                    break;

                while (shared.armed.load(std::memory_order_acquire) <
                       sequence) {
                    if (shared.abort.load(std::memory_order_acquire))
                        break;
                    bench::cpu_relax();
                }
                if (shared.abort.load(std::memory_order_acquire))
                    break;

                const auto begin = bench::now_ns();
                shared.start.store(begin, std::memory_order_release);
                event.emit();

                while (shared.done.load(std::memory_order_acquire) <
                       sequence) {
                    if (shared.abort.load(std::memory_order_acquire))
                        break;
                    bench::cpu_relax();
                }
            }

            consumer.join();

            if (cross_samples.size() != options.samples) {
                std::fprintf(stderr,
                             "event_bench: cross-thread completed %zu/%zu "
                             "samples\n",
                             cross_samples.size(), options.samples);
                ipc::Event::unbind(group);
                return 3;
            }
            bench::print_latency_us("event/cross",
                                    bench::summarize(cross_samples));
        }
    }

    ipc::Event::unbind(group);
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        return run(parse_args(argc, argv));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "event_bench: %s\n", error.what());
        return 1;
    }
}
