#pragma once

// RPC 状态码：0 成功；1..23 为框架保留（语义对齐 gRPC）；>= 100 留给业务。
// 帧头的 status 字段是 u32，跨线时按 int32_t 数值转换。

#include <cstdint>

namespace serve {
namespace rpc {

enum class Status : std::int32_t
{
    OK                    = 0,
    CANCELLED             = 1,
    UNKNOWN               = 2,
    INVALID_ARGUMENT      = 3,
    DEADLINE_EXCEEDED     = 4,
    NOT_FOUND             = 5,
    ALREADY_EXISTS        = 6,
    PERMISSION_DENIED     = 7,
    RESOURCE_EXHAUSTED    = 8,
    FAILED_PRECONDITION   = 9,
    ABORTED               = 10,
    OUT_OF_RANGE          = 11,
    UNIMPLEMENTED         = 12,
    INTERNAL              = 13,
    UNAVAILABLE           = 14,
    DATA_LOSS             = 15,
    UNAUTHENTICATED       = 16,
    IO                    = 17,
    CONNECTION_CLOSED     = 18,
    RECONNECT_FAILED      = 19,
    MAX_CONCURRENT        = 20,
    PROTOCOL              = 21,
    HANDLER_ERROR         = 22,
    BUSY                  = 23,

    USER                  = 100,
};

[[nodiscard]] const char* to_string(Status status) noexcept;

[[nodiscard]] constexpr bool is_ok(Status status) noexcept
{
    return status == Status::OK;
}

}  // namespace rpc
}  // namespace serve
