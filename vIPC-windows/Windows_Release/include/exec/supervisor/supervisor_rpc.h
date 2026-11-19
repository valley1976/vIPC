#pragma once

// supervisor 通过 serve RPC 暴露的 attach 协议。
// 控制面只做「占位/断开/复用」，Channel 数据仍在共享内存里。

#include <cstddef>
#include <cstdint>

namespace exec::supervisor {

constexpr std::uint16_t ATTACH_METHOD = 1;
constexpr std::uint16_t STATS_METHOD = 2;
constexpr std::uint16_t REPORT_METHOD = 3;
constexpr std::size_t MAX_STATS_TOPICS = 64;

enum class Attach_result : std::uint32_t
{
    OK = 0,
    BAD_REQUEST = 1,
    NO_PUBLISHER_SLOT = 2,
    NO_SUBSCRIBER_SLOT = 3,
    UNKNOWN_TOPIC = 6,
    NOT_SUBSCRIBED = 7
};

struct Attach_request
{
    std::uint32_t role = 0;   // 1 = publisher, 2 = subscriber
    char topic[64]{};
    char name[32]{};
};

struct Attach_response
{
    std::uint32_t result = 0;
    std::uint32_t reserved = 0;
    std::uint64_t token = 0;
    std::uint64_t owner_token = 0;
    std::uint64_t ring_size = 0;
    std::uint64_t subscriber_size = 0;
    std::uint64_t payload_size = 0;
    char shm_name[128]{};
    char region_name[32]{};
    std::uint32_t subscriber_index = 0;
    std::uint32_t reserved2 = 0;
};

struct Topic_stats
{
    char name[64]{};
    char region_name[32]{};
    std::uint64_t ring_size = 0;
    std::uint64_t subscriber_size = 0;
    std::uint64_t payload_size = 0;
    std::uint64_t published = 0;
    std::uint64_t committed = 0;
    std::uint64_t oldest_position = 0;
    std::uint64_t last_publish_time_ns = 0;
    std::uint64_t publisher_attached = 0;
    std::uint64_t subscriber_attached = 0;
    std::uint64_t subscriber_lag = 0;
    std::uint64_t subscriber_dropped = 0;
    std::uint64_t subscriber_reporting = 0;
};

struct Report_request
{
    char topic[64]{};
    std::uint64_t cursor = 0;
    std::uint64_t dropped = 0;
};

struct Stats_response
{
    std::uint32_t result = 0;
    std::uint32_t topic_count = 0;
    std::uint64_t pid = 0;
    std::uint64_t uptime_ms = 0;
    std::uint64_t shm_bytes = 0;
    std::uint64_t session_count = 0;
    std::uint64_t attach_total = 0;
    std::uint64_t attach_fail = 0;
    std::uint64_t recovery_total = 0;
    char metrics_shm[128]{};
    char metrics_region[32]{};
    Topic_stats topics[MAX_STATS_TOPICS]{};
};

}  // namespace exec::supervisor
