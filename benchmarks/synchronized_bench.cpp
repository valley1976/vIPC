// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

// ipc::Synchronized 锁开销 benchmark：
//   1. 单线程 apply：uncontended lock/unlock；
//   2. 单线程 try_apply：非阻塞路径；
//   3. 多线程 apply：竞争下的聚合吞吐。
//
//   synchronized_bench --samples 200000
//   synchronized_bench --threads 4 --contention-samples 50000
//   synchronized_bench --no-contention
//
// region 使用本地对齐内存，不依赖共享内存；只包含公开头 ipc/synchronized.h。

#include "ipc/synchronized.h"

#include "bench_common.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Counter
{
    std::uint64_t value = 0;
};

using Sync = ipc::Synchronized<Counter>;

struct Options
{
    std::size_t samples = 200000;
    std::size_t warmup = 5000;
    std::size_t threads = bench::default_threads();
    std::size_t contention_samples = 50000;
    bool contention = true;
};

void print_usage(const char* exe)
{
    std::printf(
        "usage: %s [options]\n"
        "  --samples N             single-thread iterations, default 200000\n"
        "  --warmup N              warmup iterations, default 5000\n"
        "  --threads N             contention threads, default min(hw, 4)\n"
        "  --contention-samples N  iterations per thread, default 50000\n"
        "  --no-contention         skip the multi-thread phase\n"
        "  -h, --help              show this help\n",
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
        else if (arg == "--warmup")
            options.warmup =
                bench::parse_size(value("--warmup"), "--warmup", true);
        else if (arg == "--threads")
            options.threads = bench::parse_size(value("--threads"), "--threads");
        else if (arg == "--contention-samples")
            options.contention_samples =
                bench::parse_size(value("--contention-samples"),
                                  "--contention-samples");
        else if (arg == "--no-contention")
            options.contention = false;
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

    if (options.samples > 100'000'000 || options.warmup > 100'000'000 ||
        options.contention_samples > 100'000'000) {
        std::fprintf(stderr, "synchronized_bench: iteration count is too "
                             "large\n");
        std::exit(2);
    }
    if (options.threads == 0 || options.threads > 256) {
        std::fprintf(stderr,
                     "synchronized_bench: --threads must be in [1, 256]\n");
        std::exit(2);
    }
    return options;
}

int run(const Options& options)
{
    alignas(Sync::required_alignment())
        std::byte storage[Sync::required_size()]{};
    const std::string name = bench::process_tag("valley_sync_bench");

    Sync sync = Sync::create_at(storage, sizeof storage,
                                Sync::Options{name, {}}, Counter{});
    const auto increment = [](Counter& counter) { ++counter.value; };

    for (std::size_t i = 0; i < options.warmup; ++i)
        sync.apply(increment);

    // ---------------------------------------------------------------------
    // uncontended apply
    // ---------------------------------------------------------------------
    const auto apply_begin = bench::now_ns();
    for (std::size_t i = 0; i < options.samples; ++i)
        sync.apply(increment);
    const auto apply_elapsed = bench::now_ns() - apply_begin;
    const double apply_ns =
        static_cast<double>(apply_elapsed) /
        static_cast<double>(options.samples);
    const double apply_per_second =
        static_cast<double>(options.samples) /
        (static_cast<double>(apply_elapsed) / 1'000'000'000.0);
    std::printf(
        "synchronized_bench: uncontended apply n=%zu ns/op=%.1f "
        "Mops/s=%.2f\n",
        options.samples, apply_ns, apply_per_second / 1'000'000.0);

    // ---------------------------------------------------------------------
    // uncontended try_apply
    // ---------------------------------------------------------------------
    std::size_t try_failures = 0;
    const auto try_begin = bench::now_ns();
    for (std::size_t i = 0; i < options.samples; ++i) {
        if (!sync.try_apply(increment))
            ++try_failures;
    }
    const auto try_elapsed = bench::now_ns() - try_begin;
    const double try_ns =
        static_cast<double>(try_elapsed) / static_cast<double>(options.samples);
    const double try_per_second =
        static_cast<double>(options.samples) /
        (static_cast<double>(try_elapsed) / 1'000'000'000.0);
    std::printf(
        "synchronized_bench: uncontended try_apply n=%zu success=%zu "
        "ns/op=%.1f Mops/s=%.2f\n",
        options.samples, options.samples - try_failures, try_ns,
        try_per_second / 1'000'000.0);

    // ---------------------------------------------------------------------
    // contention
    // ---------------------------------------------------------------------
    std::uint64_t contention_elapsed = 0;
    if (options.contention) {
        std::atomic<std::size_t> ready{0};
        std::atomic<bool> go{false};
        std::vector<std::thread> workers;
        workers.reserve(options.threads);

        for (std::size_t i = 0; i < options.threads; ++i) {
            workers.emplace_back([&sync, &ready, &go, &increment,
                                  iterations = options.contention_samples] {
                ready.fetch_add(1, std::memory_order_release);
                while (!go.load(std::memory_order_acquire))
                    bench::cpu_relax();

                for (std::size_t n = 0; n < iterations; ++n)
                    sync.apply(increment);
            });
        }

        while (ready.load(std::memory_order_acquire) < options.threads)
            bench::cpu_relax();

        const auto begin = bench::now_ns();
        go.store(true, std::memory_order_release);
        for (auto& worker : workers)
            worker.join();
        contention_elapsed = bench::now_ns() - begin;

        const auto operations = options.threads * options.contention_samples;
        const double contention_ns =
            static_cast<double>(contention_elapsed) /
            static_cast<double>(operations);
        const double contention_per_second =
            static_cast<double>(operations) /
            (static_cast<double>(contention_elapsed) / 1'000'000'000.0);
        std::printf(
            "synchronized_bench: contention threads=%zu ops=%zu ns/op=%.1f "
            "Mops/s=%.2f\n",
            options.threads, operations, contention_ns,
            contention_per_second / 1'000'000.0);
    }

    // ---------------------------------------------------------------------
    // 校验没有丢失更新
    // ---------------------------------------------------------------------
    const std::uint64_t expected =
        static_cast<std::uint64_t>(options.warmup) + options.samples +
        (options.samples - try_failures) +
        (options.contention
             ? static_cast<std::uint64_t>(options.threads) *
                   options.contention_samples
             : 0);
    const auto actual =
        sync.apply([](const Counter& counter) { return counter.value; });
    if (actual != expected) {
        std::fprintf(stderr,
                     "synchronized_bench: lost update: got %llu, expected "
                     "%llu\n",
                     static_cast<unsigned long long>(actual),
                     static_cast<unsigned long long>(expected));
        return 2;
    }

    std::printf("synchronized_bench: value=%llu ok (%.1f ms contention)\n",
                static_cast<unsigned long long>(actual),
                static_cast<double>(contention_elapsed) / 1'000'000.0);
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        return run(parse_args(argc, argv));
    } catch (const std::exception& error) {
        std::fprintf(stderr, "synchronized_bench: %s\n", error.what());
        return 1;
    }
}
