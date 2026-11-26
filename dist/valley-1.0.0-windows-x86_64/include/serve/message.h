// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "serve/export.h"

// 通用消息帧：16 字节头 + 变长 payload，IPC 与 TCP 共用。
// 只在同机同架构上传输，字节序按本机，不做转换。
//   off 0  kind     request / response / notification / hello / ping / pong
//   off 2  method   方法号（request / notification 用）
//   off 4  sequence 请求序号（response 回填）
//   off 8  status   response 状态码，其它类型为 0
//   off 12 size     payload 字节数
// 帧头不带 magic：RPC 在建连的 HELLO 载荷里校验一次协议识别字，之后只按长度解析。

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace serve {

enum class Kind : std::uint16_t
{
    REQUEST      = 1,
    RESPONSE     = 2,
    NOTIFICATION = 3,
    HELLO        = 4,  // RPC 握手：payload 是识别字 + 本端支持的方法/通知 id 列表
    PING         = 5,  // 应用层保活：sequence 原样回填到 PONG
    PONG         = 6
};

class LIBVALLEY_SERVE_EXPORT Message
{
public:
    static constexpr std::size_t HEADER_SIZE = 16;
    static constexpr std::size_t MAX_PAYLOAD = 4u * 1024u * 1024u;

    Message() = default;

    [[nodiscard]] static Message make_request(std::uint16_t method, std::uint32_t sequence,
                                              std::vector<std::byte> payload = {});
    [[nodiscard]] static Message make_response(std::uint32_t sequence, std::uint32_t status,
                                               std::vector<std::byte> payload = {},
                                               std::uint16_t method = 0);
    [[nodiscard]] static Message make_notification(std::uint16_t method,
                                                   std::vector<std::byte> payload = {});
    [[nodiscard]] static Message make_hello(std::vector<std::byte> payload);

    [[nodiscard]] static Message make_ping(std::uint32_t sequence);
    [[nodiscard]] static Message make_pong(std::uint32_t sequence);

    [[nodiscard]] Kind kind() const noexcept { return kind_; }
    [[nodiscard]] std::uint16_t method() const noexcept { return method_; }
    [[nodiscard]] std::uint32_t sequence() const noexcept { return sequence_; }
    [[nodiscard]] std::uint32_t status() const noexcept { return status_; }
    [[nodiscard]] const std::vector<std::byte>& payload() const noexcept { return payload_; }
    [[nodiscard]] std::vector<std::byte>& payload() noexcept { return payload_; }

    [[nodiscard]] std::vector<std::byte> serialize() const;
    // 零额外分配：调用方提供 frame_size() 字节的目标缓冲
    [[nodiscard]] std::size_t frame_size() const noexcept;
    void serialize_into(std::byte* dest) const noexcept;

    // 从连续字节里解析一帧：数据不足或格式非法返回 nullopt，且 consumed 置 0；
    // 成功时 consumed 是这一帧的总长度（头 + payload）。
    [[nodiscard]] static std::optional<Message> parse(const std::byte* data, std::size_t size,
                                                      std::size_t& consumed);

    // 头部的 kind / size 是否合法；用于区分「半包」和「坏帧」
    [[nodiscard]] static bool is_valid_header(const std::byte* data, std::size_t size) noexcept;

private:
    Kind kind_{Kind::REQUEST};
    std::uint16_t method_{0};
    std::uint32_t sequence_{0};
    std::uint32_t status_{0};
    std::vector<std::byte> payload_;
};

}  // namespace serve
