// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "base/log/sink.h"

namespace base {

struct Tcp_options
{
    std::string host = "127.0.0.1";
    std::string port = "9999";
};

// 同步 TCP sink；调用方应使用 Async_sink 包装，避免阻塞业务线程。
class Tcp_sink final : public Sink
{
public:
    explicit Tcp_sink(Tcp_options options);
    ~Tcp_sink() override;

    Tcp_sink(const Tcp_sink&) = delete;
    Tcp_sink& operator=(const Tcp_sink&) = delete;

    void write(const Log_record& record) override;
    void flush() override;
    void shutdown() noexcept;

private:
    bool connect_locked();
    bool send_locked(const Log_record& record);
    void close_locked() noexcept;

    std::mutex mutex_;
    Tcp_options options_;
    std::intptr_t socket_ = -1;
};

}  // namespace base
