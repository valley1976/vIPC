#pragma once

#include <cstdint>
#include <chrono>

namespace valley {
namespace serve {
namespace ipc {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using Byte = u8;
using Ms   = std::chrono::milliseconds;

using Method   = u16;
using Sequence = u32;
using Status   = u32;

using Payload       = const void*;
using Payload_size  = u16;

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
    kCancelAndReplace,
    kCancelAndReplaceIfExpireFor
};

struct Busy_option 
{
    Busy_policy policy;
    Ms expire_for;

    Busy_option(Busy_policy p) : policy(p),
        expire_for()
    {}

    Busy_option(Busy_policy p, const Ms time) : policy(p),
        expire_for(time)
    {}
};

}
}
}