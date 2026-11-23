// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

// ipc::Channel 吞吐 benchmark：一次 publish，所有 subscriber 各读一次。
//
//   channel_bench --samples 200000 --subscribers 1
//   channel_bench --subscribers 4 --ring-size 4096
//   channel_bench --payload 256 --samples 100000    # chunk 模式
//
// 统计测得的每轮迭代耗时（publish + N 次 read）和每秒迭代/值数量。
// 默认使用公开的 ipc/channel.h，不包含任何实现头。

#include "ipc/channel.h"

#include "bench_common.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options
{
    std::size_t samples = 200000;
    std::size_t warmup = 1000;
    std::size_t ring_size = 1024;
    std::size_t subscribers = 1;
    std::size_t payload = 0;
};

void print_usage(const char* exe)
{
    std::printf(
        "usage: %s [options]\n"
        "  --samples N       measured iterations, default 200000\n"
        "  --warmup N        warmup iterations, default 1000\n"
        "  --ring-size N     channel ring size, default 1024\n"
        "  --subscribers N   subscribers per channel, default 1 (1..63)\n"
        "  --payload N       chunk payload bytes, default 0 (scalar mode)\n"
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
        else if (arg == "--warmup")
            options.warmup =
                bench::parse_size(value("--warmup"), "--warmup", true);
        else if (arg == "--ring-size")
            options.ring_size =
                bench::parse_size(value("--ring-size"), "--ring-size");
        else if (arg == "--subscribers")
            options.subscribers =
                bench::parse_size(value("--subscribers"), "--subscribers");
        else if (arg == "--payload")
            options.payload =
                bench::parse_size(value("--payload"), "--payload", true);
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

    if (options.ring_size <= 1) {
        std::fprintf(stderr, "channel_bench: --ring-size must be > 1\n");
        std::exit(2);
    }
    if (options.subscribers == 0 || options.subscribers > 63) {
        std::fprintf(stderr,
                     "channel_bench: --subscribers must be in [1, 63]\n");
        std::exit(2);
    }
    if (options.payload > 16u * 1024u * 1024u) {
        std::fprintf(stderr, "channel_bench: --payload is too large\n");
        std::exit(2);
    }
    return options;
}

int run(const Options& options)
{
    ipc::Channel::Config config;
    config.ring_size = options.ring_size;
    config.subscriber_size = options.subscribers;
    config.payload_size = options.payload;
    if (!ipc::Channel::valid_config(config)) {
        std::fprintf(stderr, "channel_bench: invalid channel configuration\n");
        return 2;
    }

    ipc::Channel channel{config};
    std::vector<std::unique_ptr<ipc::Channel::Subscriber>> subscribers;
    std::vector<std::unique_ptr<ipc::Channel::Reader>> readers;
    subscribers.reserve(options.subscribers);
    readers.reserve(options.subscribers);

    for (std::size_t i = 0; i < options.subscribers; ++i) {
        auto subscriber = channel.subscribe();
        if (!subscriber) {
            std::fprintf(stderr,
                         "channel_bench: subscribe failed at subscriber %zu\n",
                         i);
            return 1;
        }
        readers.push_back(subscriber->make_reader());
        if (!readers.back()) {
            std::fprintf(stderr,
                         "channel_bench: make_reader failed at subscriber "
                         "%zu\n",
                         i);
            return 1;
        }
        subscribers.push_back(std::move(subscriber));
    }

    std::uint64_t checksum = 0;
    const auto run_once = [&](std::uint64_t value) -> bool {
        if (options.payload == 0) {
            if (!channel.publish(value))
                return false;
        } else {
            auto chunk = channel.acquire_chunk();
            if (!chunk)
                return false;
            std::memset(chunk->data(), static_cast<int>(value & 0xffu),
                        options.payload);
            chunk->set_size(options.payload);
            if (!channel.publish(std::move(*chunk)))
                return false;
        }

        for (const auto& reader : readers) {
            if (!reader->try_read())
                return false;
            if (options.payload == 0) {
                checksum += reader->value();
            } else {
                if (reader->payload_size() != options.payload)
                    return false;
                const auto* bytes =
                    static_cast<const unsigned char*>(reader->payload());
                if (bytes[0] != static_cast<unsigned char>(value & 0xffu))
                    return false;
                checksum += bytes[0];
            }
            reader->unpin();
        }
        return true;
    };

    for (std::uint64_t i = 0; i < options.warmup; ++i) {
        if (!run_once(i)) {
            std::fprintf(stderr, "channel_bench: warmup failed at %llu\n",
                         static_cast<unsigned long long>(i));
            return 2;
        }
    }

    checksum = 0;
    const auto begin = bench::now_ns();
    for (std::uint64_t i = 0; i < options.samples; ++i) {
        const auto value = static_cast<std::uint64_t>(options.warmup) + i;
        if (!run_once(value)) {
            std::fprintf(stderr, "channel_bench: publish/read failed at %llu\n",
                         static_cast<unsigned long long>(i));
            return 2;
        }
    }
    const auto elapsed = bench::now_ns() - begin;

    const double seconds =
        static_cast<double>(elapsed) / 1'000'000'000.0;
    const double ns_per_iteration =
        seconds > 0.0
            ? static_cast<double>(elapsed) / static_cast<double>(options.samples)
            : 0.0;
    const double iterations_per_second =
        seconds > 0.0 ? static_cast<double>(options.samples) / seconds : 0.0;
    const double values_per_second =
        iterations_per_second * static_cast<double>(options.subscribers);

    std::printf(
        "channel_bench: ring=%zu subscribers=%zu payload=%zu samples=%zu "
        "warmup=%zu\n",
        options.ring_size, options.subscribers, options.payload,
        options.samples, options.warmup);
    std::printf(
        "channel_bench: iterations=%zu ns/iter=%.1f Miter/s=%.2f "
        "Mvalues/s=%.2f checksum=%llu\n",
        options.samples, ns_per_iteration, iterations_per_second / 1'000'000.0,
        values_per_second / 1'000'000.0,
        static_cast<unsigned long long>(checksum));

    if (options.payload == 0) {
        std::uint64_t expected = 0;
        for (std::uint64_t i = 0; i < options.samples; ++i)
            expected += static_cast<std::uint64_t>(options.warmup) + i;
        expected *= options.subscribers;
        if (checksum != expected) {
            std::fprintf(stderr,
                         "channel_bench: checksum mismatch: got %llu, "
                         "expected %llu\n",
                         static_cast<unsigned long long>(checksum),
                         static_cast<unsigned long long>(expected));
            return 2;
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    const Options options = parse_args(argc, argv);
    try {
        return run(options);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "channel_bench: %s\n", error.what());
        return 1;
    }
}
