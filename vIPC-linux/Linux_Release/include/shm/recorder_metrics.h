#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#pragma warning(push)
// alignas(64) 是 cache-line 分隔，C4324 是本设计预期的填充。
#pragma warning(disable : 4324)
#endif

namespace shm {

inline constexpr char RECORDER_METRICS_REGION[] = "recorder_metrics";
inline constexpr char CHANNEL_REGISTRY_REGION[] = "channel_registry";
inline constexpr char RECORDER_METRICS_SHM_PREFIX[] = "dsh-metrics-";

inline constexpr std::uint64_t RECORDER_METRICS_MAGIC = 0x4D45545249435331ULL;
inline constexpr std::uint32_t RECORDER_METRICS_VERSION = 1;
inline constexpr std::size_t MAX_RECORDER_METRIC_SLOTS = 64;
inline constexpr std::size_t MAX_BUNDLES_PER_RECORDER = 8;
inline constexpr std::size_t MAX_REGISTRY_TOPICS = 64;
inline constexpr std::size_t MAX_REGISTRY_SUBSCRIBERS = 63;
inline constexpr std::uint64_t CHANNEL_REGISTRY_MAGIC = 0x5245475354525931ULL;
inline constexpr std::uint32_t CHANNEL_REGISTRY_VERSION = 1;

enum class Recorder_metric_state : std::uint32_t
{
    EMPTY = 0,
    ACTIVE = 1,
    STALE = 2
};

struct Bundle_metric_slot
{
    char name[32]{};
    std::uint32_t mode = 0;  // 0 = sync, 1 = async
    std::uint32_t reserved = 0;
    std::uint64_t written = 0;
    std::uint64_t bytes_written = 0;
    std::uint64_t segment_count = 0;
    std::uint64_t queue_depth_bytes = 0;
    std::uint64_t queue_high_watermark = 0;
    std::uint64_t dropped = 0;
    std::uint64_t unwritten = 0;
    std::uint64_t queue_overflow = 0;
    std::uint64_t pre_roll_truncated = 0;
    std::uint64_t late_triggers = 0;
    std::uint64_t publish_failures = 0;
    std::uint64_t latency_p50_us = 0;
    std::uint64_t latency_p95_us = 0;
    std::uint64_t latency_p99_us = 0;
    std::uint64_t latency_max_us = 0;
    std::uint64_t trigger_total = 0;
    std::uint32_t active_triggers = 0;
    std::uint32_t mode_switch_pending = 0;
};

struct alignas(64) Recorder_metric_slot
{
    std::atomic<std::uint32_t> sequence{0};
    std::atomic<std::uint32_t> state{0};

    std::uint64_t pid = 0;
    std::uint64_t heartbeat = 0;
    std::uint64_t timestamp_ns = 0;
    std::uint64_t started_at_ns = 0;
    std::uint32_t error_code = 0;
    std::uint32_t bundle_count = 0;
    char recorder_name[32]{};
    Bundle_metric_slot bundles[MAX_BUNDLES_PER_RECORDER]{};
};

struct Recorder_metrics_header
{
    std::uint64_t magic = RECORDER_METRICS_MAGIC;
    std::uint32_t version = RECORDER_METRICS_VERSION;
    std::uint32_t slot_count = MAX_RECORDER_METRIC_SLOTS;
    std::uint64_t epoch = 0;
    std::uint64_t region_size = 0;
};

struct Recorder_metrics_region
{
    alignas(64) Recorder_metrics_header header;
    Recorder_metric_slot slots[MAX_RECORDER_METRIC_SLOTS];
};

struct Registry_subscriber
{
    char name[32]{};
    std::uint32_t index = 0;
    std::uint32_t reserved = 0;
};

struct Registry_topic
{
    char topic[64]{};
    char shm_name[128]{};
    char region_name[32]{};
    std::uint32_t subscriber_count = 0;
    std::uint32_t reserved = 0;
    Registry_subscriber subscribers[MAX_REGISTRY_SUBSCRIBERS]{};
};

struct Channel_registry_header
{
    std::uint64_t magic = CHANNEL_REGISTRY_MAGIC;
    std::uint32_t version = CHANNEL_REGISTRY_VERSION;
    std::uint32_t topic_count = 0;
    std::uint64_t pid = 0;
    std::uint64_t started_at_ns = 0;
};

struct Channel_registry_region
{
    Channel_registry_header header;
    Registry_topic topics[MAX_REGISTRY_TOPICS];
};

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "recorder metrics requires lock-free 32-bit atomics");

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}  // namespace shm
