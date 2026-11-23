// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

// serve::Server / serve::Client RPC 往返 benchmark：串行 ping-pong，
// 统计一次 request -> response 的往返时延。
//
//   serve_bench                          # IPC，双 loop/双线程
//   serve_bench --tcp                    # TCP，双 loop/双线程
//   serve_bench --same-loop              # 单 loop，隔离线程唤醒成本
//   serve_bench --payload 256 --samples 50000
//   serve_bench --sweep                  # 16B/64B/256B/1KB/4KB 扫描
//
// 只使用公开的 serve/serve.h 门面，不包含 serve 的 src/ 内部头。

#include "serve/serve.h"

#include "bench_common.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

enum class Transport
{
    IPC,
    TCP
};

struct Options
{
    Transport transport = Transport::IPC;
    bool same_loop = false;
    bool sweep = false;
    std::size_t samples = 20000;
    std::size_t warmup = 2000;
    std::size_t payload = 16;
};

void print_usage(const char* exe)
{
    std::printf(
        "usage: %s [options]\n"
        "  --ipc              use IPC transport (default)\n"
        "  --tcp              use TCP transport\n"
        "  --same-loop        run server and client on one Event_loop\n"
        "  --sweep            sweep payload sizes 16/64/256/1024/4096\n"
        "  --samples N        measured round trips, default 20000\n"
        "  --warmup N         warmup round trips, default 2000\n"
        "  --payload N        request payload bytes, default 16\n"
        "  -h, --help         show this help\n",
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

        if (arg == "--ipc")
            options.transport = Transport::IPC;
        else if (arg == "--tcp")
            options.transport = Transport::TCP;
        else if (arg == "--same-loop")
            options.same_loop = true;
        else if (arg == "--sweep")
            options.sweep = true;
        else if (arg == "--samples")
            options.samples = bench::parse_size(value("--samples"), "--samples");
        else if (arg == "--warmup")
            options.warmup =
                bench::parse_size(value("--warmup"), "--warmup", true);
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

    if (options.samples > 10'000'000 || options.warmup > 10'000'000) {
        std::fprintf(stderr, "serve_bench: sample count is too large\n");
        std::exit(2);
    }
    if (options.payload > serve::Message::MAX_PAYLOAD) {
        std::fprintf(stderr,
                     "serve_bench: --payload exceeds Message::MAX_PAYLOAD\n");
        std::exit(2);
    }
    return options;
}

struct Runner
{
    std::shared_ptr<serve::Client> client;
    std::vector<std::byte> payload;
    std::size_t warmup_left = 0;
    std::size_t target = 0;
    std::vector<std::uint64_t> samples;
    std::atomic<bool> done{false};
    std::atomic<bool> failed{false};

    void start() { step_once(); }

    void step_once()
    {
        const auto begin = bench::now_ns();
        const bool accepted = client->request(
            1, payload,
            [this, begin](serve::rpc::Status status,
                          std::vector<std::byte> body) {
                (void)body;
                if (status != serve::rpc::Status::OK) {
                    std::fprintf(stderr,
                                 "serve_bench: request failed: %s\n",
                                 serve::rpc::to_string(status));
                    failed.store(true, std::memory_order_release);
                    done.store(true, std::memory_order_release);
                    return;
                }

                const auto end = bench::now_ns();
                if (warmup_left > 0) {
                    --warmup_left;
                } else {
                    samples.push_back(end - begin);
                }

                if (samples.size() >= target) {
                    done.store(true, std::memory_order_release);
                    return;
                }
                step_once();
            },
            std::chrono::seconds{5});

        if (!accepted) {
            std::fprintf(stderr, "serve_bench: request was not accepted\n");
            failed.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        }
    }
};

bool run_case(const Options& options, std::size_t payload,
              bench::Summary& summary)
{
    serve::Event_loop server_loop("bench_server");
    std::unique_ptr<serve::Event_loop> client_loop_storage;
    if (!options.same_loop)
        client_loop_storage = std::make_unique<serve::Event_loop>("bench_client");
    serve::Event_loop& client_loop =
        options.same_loop ? server_loop : *client_loop_storage;

    server_loop.start();
    if (!options.same_loop)
        client_loop.start();

    const auto stop_loops = [&] {
        if (!options.same_loop)
            client_loop.stop();
        server_loop.stop();
    };

    const std::string ipc_name = bench::process_tag("serve_bench") + "_" +
                                 std::to_string(payload);
    const std::string address =
        options.transport == Transport::TCP ? "tcp://127.0.0.1:0"
                                            : "ipc://" + ipc_name;

    // runner 先声明，保证它在 client/server 之后析构；回调里捕获的是
    // runner 的裸 this，不能让 runner 先于连接对象消失。
    std::shared_ptr<Runner> runner;
    std::shared_ptr<serve::Server> server;
    try {
        server = std::make_shared<serve::Server>(server_loop, address);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "serve_bench: server address error: %s\n",
                     error.what());
        stop_loops();
        return false;
    }

    const bool registered = server->register_method(
        1, [](const serve::Session_base::Ptr&, serve::Message& request,
              std::vector<std::byte>& response) {
            response = request.payload();
        });
    if (!registered) {
        std::fprintf(stderr, "serve_bench: register_method failed\n");
        stop_loops();
        return false;
    }

    if (!server->start()) {
        std::fprintf(stderr, "serve_bench: server start failed: %s\n",
                     server->address_error().message().c_str());
        stop_loops();
        return false;
    }

    std::shared_ptr<serve::Client> client;
    try {
        client = std::make_shared<serve::Client>(client_loop,
                                                 server->address());
    } catch (const std::exception& error) {
        std::fprintf(stderr, "serve_bench: client address error: %s\n",
                     error.what());
        stop_loops();
        return false;
    }

    runner = std::make_shared<Runner>();
    runner->client = client;
    runner->payload.assign(payload, std::byte{0x5a});
    runner->warmup_left = options.warmup;
    runner->target = options.samples;
    runner->samples.reserve(options.samples);

    const std::weak_ptr<Runner> weak_runner = runner;
    client->set_error_handler([weak_runner](const std::error_code& error) {
        std::fprintf(stderr, "serve_bench: connect error: %s\n",
                     error.message().c_str());
        if (auto state = weak_runner.lock()) {
            state->failed.store(true, std::memory_order_release);
            state->done.store(true, std::memory_order_release);
        }
    });
    client->set_on_disconnected([weak_runner] {
        if (auto state = weak_runner.lock()) {
            if (!state->done.load(std::memory_order_acquire)) {
                state->failed.store(true, std::memory_order_release);
                state->done.store(true, std::memory_order_release);
            }
        }
    });
    client->set_on_connected([weak_runner] {
        if (auto state = weak_runner.lock())
            state->start();
    });
    client->connect_async(std::chrono::seconds{5});

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds{120};
    while (!runner->done.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    client->disconnect();
    stop_loops();

    const bool completed = runner->done.load(std::memory_order_acquire);
    const bool failed = runner->failed.load(std::memory_order_acquire);
    // loop 已停，回调不会再写 samples；此时再拷贝，避免与回调并发。
    std::vector<std::uint64_t> samples = runner->samples;

    if (!completed) {
        std::fprintf(stderr, "serve_bench: timed out waiting for responses\n");
        return false;
    }
    if (failed) {
        std::fprintf(stderr, "serve_bench: benchmark failed\n");
        return false;
    }
    if (samples.size() != options.samples) {
        std::fprintf(stderr, "serve_bench: got %zu/%zu samples\n",
                     samples.size(), options.samples);
        return false;
    }

    summary = bench::summarize(std::move(samples));
    return true;
}

void print_result(const Options& options, std::size_t payload,
                  const bench::Summary& summary)
{
    const char* transport =
        options.transport == Transport::TCP ? "tcp" : "ipc";
    const char* loop = options.same_loop ? "same" : "split";
    std::printf(
        "serve_bench: transport=%s loop=%s payload=%zu samples=%zu "
        "warmup=%zu\n",
        transport, loop, payload, options.samples, options.warmup);
    bench::print_latency_us("serve/rpc", summary);
    const double round_trips =
        summary.mean > 0.0 ? 1'000'000'000.0 / summary.mean : 0.0;
    std::printf("serve_bench: %.0f rt/s (serial)\n", round_trips);
}

}  // namespace

int main(int argc, char** argv)
{
    const Options options = parse_args(argc, argv);
    try {
        if (options.sweep) {
            for (const std::size_t payload :
                 {std::size_t{16}, std::size_t{64}, std::size_t{256},
                  std::size_t{1024}, std::size_t{4096}}) {
                bench::Summary summary;
                if (!run_case(options, payload, summary))
                    return 1;
                print_result(options, payload, summary);
            }
            return 0;
        }

        bench::Summary summary;
        if (!run_case(options, options.payload, summary))
            return 1;
        print_result(options, options.payload, summary);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "serve_bench: %s\n", error.what());
        return 1;
    }
}
