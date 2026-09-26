#pragma once

#include <cstdint>

namespace valley {
namespace ipc {
namespace serve {

struct Protocol
{
    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using Byte = u8;

    // 协议主版本（高 4 位）+ 次版本（低 4 位）
    static constexpr u8 kPROTOCOL_VERSION_MAJOR = 1;
    static constexpr u8 kPROTOCOL_VERSION_MINOR = 0;

    // 0x10
    static constexpr u8  kPROTOCOL_VERSION = u8((kPROTOCOL_VERSION_MAJOR << 4) | (kPROTOCOL_VERSION_MINOR & 0x0F));
    
    // vRPC
    static constexpr u32 kPROTOCOL_MAGIC = u32(u8('v')) | u32(u8('R')) << 8 | u32(u8('P')) << 16 | u32(u8('C')) << 24;

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

    // ------------------------------------------------------------------
    // 消息类型
    // ------------------------------------------------------------------
    enum class Type : u8 {
        kREQUEST,
        kRESPONSE,
        kNOTIFICATION,
    };
};

}
}
}