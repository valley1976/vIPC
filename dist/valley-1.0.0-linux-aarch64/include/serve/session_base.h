// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "serve/export.h"

// 分帧会话的传输无关接口：rpc 层只依赖这个基类，不关心底层是命名管道、本机套接字
// 还是 TCP。具体实现是 ipc::Basic_session<Stream>。
// 线程约定与 Buffered_stream 一致：send()/close() 在 loop 线程调用。

#include <cstdint>
#include <functional>
#include <memory>
#include <system_error>

#include "serve/message.h"

namespace serve {

class LIBVALLEY_SERVE_EXPORT Session_base
{
public:
    using Ptr = std::shared_ptr<Session_base>;

    struct Handler
    {
        std::function<void(const Ptr&)> on_connected;
        std::function<void(const Ptr&)> on_disconnected;
        std::function<void(const Ptr&, Message&)> on_message;
        std::function<void(const Ptr&, const std::error_code&)> on_error;
    };

    Session_base()                                = default;
    virtual ~Session_base()                       = default;

    Session_base(const Session_base&)            = delete;
    Session_base& operator=(const Session_base&) = delete;

    [[nodiscard]] virtual std::uint64_t id() const noexcept        = 0;
    [[nodiscard]] virtual bool is_connected() const noexcept       = 0;
    [[nodiscard]] virtual bool send(const Message& message)   = 0;

    virtual void open()        = 0;
    virtual void close()       = 0;
    virtual void close_async() = 0;

    virtual void set_handler(std::shared_ptr<const Handler> handler)     = 0;
    virtual void set_close_handler(std::function<void(std::uint64_t)>)   = 0;
};

}  // namespace serve
