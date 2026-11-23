// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "serve/serve.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

int main()
{
    serve::Event_loop loop("minimal_serve");
    loop.start();

    try {
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        const std::string address = "ipc://minimal_serve_" + suffix;

        std::atomic<bool> done{false};
        std::atomic<bool> ok{false};
        std::atomic<bool> failed{false};
        std::vector<std::byte> reply;
        const std::vector<std::byte> payload{std::byte{'p'}, std::byte{'i'},
                                             std::byte{'n'}, std::byte{'g'}};

        auto server = std::make_shared<serve::Server>(loop, address);
        const bool registered = server->register_method(
            1, [](const serve::Session_base::Ptr&, serve::Message& request,
                  std::vector<std::byte>& response) {
                response = request.payload();
            });
        if (!registered) {
            std::fprintf(stderr, "minimal_serve: register_method failed\n");
            server.reset();
            loop.stop();
            return 1;
        }
        if (!server->start()) {
            std::fprintf(stderr, "minimal_serve: server start failed: %s\n",
                         server->address_error().message().c_str());
            server.reset();
            loop.stop();
            return 1;
        }

        auto client = std::make_shared<serve::Client>(loop, server->address());
        std::weak_ptr<serve::Client> weak_client = client;
        client->set_on_connected([weak_client, &done, &ok, &failed, &reply,
                                  payload] {
            auto connected = weak_client.lock();
            if (!connected) {
                failed.store(true, std::memory_order_release);
                done.store(true, std::memory_order_release);
                return;
            }
            const bool accepted = connected->request(
                1, payload,
                [&done, &ok, &failed, &reply](serve::rpc::Status status,
                                              std::vector<std::byte> body) {
                    if (status == serve::rpc::Status::OK) {
                        reply = std::move(body);
                        ok.store(true, std::memory_order_release);
                    } else {
                        failed.store(true, std::memory_order_release);
                    }
                    done.store(true, std::memory_order_release);
                },
                std::chrono::seconds{3});
            if (!accepted) {
                failed.store(true, std::memory_order_release);
                done.store(true, std::memory_order_release);
            }
        });
        client->set_on_disconnected([&done, &failed] {
            if (!done.load(std::memory_order_acquire)) {
                failed.store(true, std::memory_order_release);
                done.store(true, std::memory_order_release);
            }
        });
        client->set_error_handler([&done, &failed](const std::error_code& error) {
            std::fprintf(stderr, "minimal_serve: connect error: %s\n",
                         error.message().c_str());
            failed.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        });
        client->connect_async(std::chrono::seconds{2});

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds{5};
        while (!done.load(std::memory_order_acquire) &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }

        client->disconnect();
        client.reset();
        server.reset();
        loop.stop();

        if (!done.load(std::memory_order_acquire)) {
            std::fprintf(stderr, "minimal_serve: timed out\n");
            return 1;
        }
        if (failed.load(std::memory_order_acquire)) {
            std::fprintf(stderr, "minimal_serve: request failed\n");
            return 1;
        }
        if (!ok.load(std::memory_order_acquire) || reply != payload) {
            std::fprintf(stderr, "minimal_serve: unexpected response\n");
            return 1;
        }

        std::printf("serve echo: ping\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "minimal_serve: %s\n", error.what());
        loop.stop();
        return 1;
    }
}
