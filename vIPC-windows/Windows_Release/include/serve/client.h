#pragma once

// 统一入口的客户端：构造时传地址（`ipc://name` 或 `tcp://host:port`），
// 内部自动选择 IPC / TCP 实现；接口与 rpc::Ipc_client / rpc::Tcp_client 一致。
// 线程约定：connect_async/disconnect/register_* 任意线程；request/notify 任意线程（内部投递）；
// 回调都在 loop 线程。生命周期：必须由 std::shared_ptr 托管。

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

class Client : public std::enable_shared_from_this<Client>
{
public:
    using Session_ptr      = Session_base::Ptr;
    using Handler          = ::serve::Handler;
    using Response_handler = ::serve::Response_handler;

    // 地址非法或给了不可连接的通配地址时抛 std::invalid_argument
    Client(Event_loop& loop, std::string address);
    ~Client();

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&)                 = delete;
    Client& operator=(Client&&)      = delete;

    void connect_async();
    // timeout > 0 时超过该时长仍未连上就报 std::errc::timed_out
    void connect_async(std::chrono::milliseconds timeout);
    void disconnect();
    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] bool is_ready() const;
    [[nodiscard]] std::uint64_t session_id() const;

    [[nodiscard]] bool register_method(std::uint16_t method, Handler handler);
    [[nodiscard]] bool register_notification(std::uint16_t method, Handler handler);

    [[nodiscard]] bool request(std::uint16_t method, std::vector<std::byte> payload,
                               Response_handler on_response,
                               std::chrono::milliseconds timeout = std::chrono::seconds(3),
                               bool replace_inflight = false);
    [[nodiscard]] bool notify(std::uint16_t method, std::vector<std::byte> payload);

    // 在飞请求上限；0 = 不限制
    void setup_max_inflight(std::size_t limit) noexcept;
    // 应用层保活：interval <= 0 表示关闭（默认）
    void setup_keepalive(std::chrono::milliseconds interval,
                         std::chrono::milliseconds timeout);

    void set_on_connected(std::function<void()> handler);
    void set_on_disconnected(std::function<void()> handler);
    void set_error_handler(std::function<void(const std::error_code&)> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace serve
