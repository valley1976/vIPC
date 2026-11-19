#pragma once

// 统一入口的服务端：构造时传地址（`ipc://name` 或 `tcp://host:port`），
// 内部自动选择 IPC / TCP 实现；对外只暴露 RPC 语义。
// 生命周期：必须由 std::shared_ptr 托管；start()/stop() 在 loop 线程或 loop 启动前调用。

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "serve/address.h"
#include "serve/event_loop.h"
#include "serve/session_base.h"
#include "serve/types.h"

namespace serve {

class Server : public std::enable_shared_from_this<Server>
{
public:
    using Session_ptr      = Session_base::Ptr;
    using Handler           = ::serve::Handler;
    using Response_handler  = ::serve::Response_handler;
    using Broadcast_handler = ::serve::Broadcast_handler;

    // 地址非法时抛 std::invalid_argument
    Server(Event_loop& loop, std::string address);
    ~Server();

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&)                 = delete;
    Server& operator=(Server&&)      = delete;

    [[nodiscard]] bool start();
    void stop();
    // 优雅关闭：停止接收新连接，等在飞请求完成（或超时）再断开会话
    void stop_graceful(std::chrono::milliseconds timeout,
                       std::function<void()> on_done = {});
    [[nodiscard]] bool is_started() const noexcept;

    // 规范化后的监听地址；TCP 端口传 0 时这里回填实际端口
    [[nodiscard]] std::string address() const;
    [[nodiscard]] int port() const noexcept;
    [[nodiscard]] const std::error_code& address_error() const noexcept;

    [[nodiscard]] std::size_t connected_sessions() const;
    [[nodiscard]] bool is_ready(std::uint64_t session_id) const;

    [[nodiscard]] bool register_method(std::uint16_t method, Handler handler);
    [[nodiscard]] bool register_notification(std::uint16_t method, Handler handler);

    [[nodiscard]] bool request(std::uint64_t session_id, std::uint16_t method,
                               std::vector<std::byte> payload, Response_handler on_response,
                               std::chrono::milliseconds timeout = std::chrono::seconds(3),
                               bool replace_inflight = false);
    [[nodiscard]] bool notify(std::uint64_t session_id, std::uint16_t method,
                              std::vector<std::byte> payload);

    [[nodiscard]] std::size_t broadcast_notify(std::uint16_t method,
                                               std::vector<std::byte> payload);
    [[nodiscard]] std::size_t broadcast_request(
        std::uint16_t method, std::vector<std::byte> payload, Broadcast_handler on_response,
        std::chrono::milliseconds timeout = std::chrono::seconds(3));

    // 连接数 / 每连接在飞请求上限；0 = 不限制
    void setup_max_sessions(std::size_t limit) noexcept;
    void setup_max_inflight(std::size_t limit) noexcept;
    // 应用层保活：interval <= 0 表示关闭（默认）
    void setup_keepalive(std::chrono::milliseconds interval,
                         std::chrono::milliseconds timeout);

    void set_on_connected(std::function<void(const Session_ptr&)> handler);
    void set_on_disconnected(std::function<void(const Session_ptr&)> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace serve
