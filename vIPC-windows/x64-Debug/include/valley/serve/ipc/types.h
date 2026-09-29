#pragma once

#include <cstdint>

namespace valley {
namespace serve {
namespace ipc {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using Byte = u8;

// ------------------------------------------------------------------
// payload 编码格式
// ------------------------------------------------------------------
enum class Encoding : u8 {
    RAW = 0,
    MSGPACK,
    PROTOBUF,
    JSON,
    CAPNP,
    FLATBUF
};

enum class Flags : u8
{
    kNone,
    kCompressed,
    kStreaming,
    kAck,
    kFinal,
};

enum class Busy_policy {
    kRejectIfBusy,
    kCancelAndReplace
};

}
}
}