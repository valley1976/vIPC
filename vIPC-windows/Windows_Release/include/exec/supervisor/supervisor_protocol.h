#pragma once

#include <cstdint>

namespace exec::supervisor {

inline constexpr std::uint32_t PROTOCOL_MAGIC = 0x44534843;  // "DSHC"
inline constexpr std::uint32_t PROTOCOL_VERSION = 2;

enum class Role : std::uint32_t {
    PUBLISHER = 1,
    SUBSCRIBER = 2
};

enum class Result : std::uint32_t {
    OK = 0,
    BAD_REQUEST = 1,
    NO_PUBLISHER_SLOT = 2,
    NO_SUBSCRIBER_SLOT = 3,
    INCOMPATIBLE = 4,
    INTERNAL_ERROR = 5,
    UNKNOWN_TOPIC = 6,
    NOT_SUBSCRIBED = 7
};

struct Hello {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t role;
    std::uint32_t reserved;
    char channel[64];   // topic name
    char name[32];      // subscriber/publisher 名称（subscriber 用于白名单）
};

struct Assign {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t result;
    std::uint32_t reserved;
    std::uint64_t token;
    std::uint64_t owner_token;
    std::uint64_t ring_size;
    std::uint64_t subscriber_size;
    char shm_name[128];
};

struct Goodbye {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t reason;
    std::uint32_t reserved;
};

struct Fence {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t reason;
    std::uint32_t reserved;
};

}  // namespace exec::supervisor
