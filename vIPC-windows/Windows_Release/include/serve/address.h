#pragma once

// 传输地址：`ipc://name` 或 `tcp://host:port`。
// 解析是纯函数，不依赖 asio 之外的任何东西；解析结果规范化后再交给传输层。

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace serve {

enum class Transport
{
    IPC,
    TCP
};

class Address
{
public:
    Address() = default;

    Address(Transport transport, std::string target, std::uint16_t port = 0)
        : transport(transport)
        , target(std::move(target))
        , port(port)
    {
    }

    Transport transport{Transport::IPC};

    // IPC：Windows 为管道名，POSIX 为文件系统路径（相对名会规范化为 /tmp/ 下）；
    // TCP：IP 字面量（IPv6 不带方括号）。
    std::string target;
    std::uint16_t port{0};

    // 语法错误返回 nullopt；不做「客户端不能连通配地址」这类语义校验。
    [[nodiscard]] static std::optional<Address> parse(std::string_view text);

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] bool is_ipc() const noexcept { return transport == Transport::IPC; }
    [[nodiscard]] bool is_tcp() const noexcept { return transport == Transport::TCP; }
};

}  // namespace serve
