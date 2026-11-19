#pragma once

// 运行时容量版单生产者/多订阅者通道：公开 API。
// 实现与 References 布局在 ipc/src/ 下，不对外暴露。

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace ipc {

class Channel {
public:
    using Value = std::uint64_t;

    static constexpr std::size_t SLOT_ALIGN = 64;
    static constexpr std::size_t CACHE_LINE = 64;

    struct Config {
        std::size_t ring_size = 1024;
        std::size_t subscriber_size = 63;
        // 0 表示旧版 scalar 模式；>0 时为每个 slot 附带固定 payload chunk。
        std::size_t payload_size = 0;
    };

    struct Stats {
        std::uint64_t published = 0;
        std::uint64_t committed = 0;
        std::uint64_t oldest_position = 0;
        std::uint64_t last_publish_time_ns = 0;
        std::uint64_t publish_failures = 0;
        std::size_t capacity = 0;
        std::size_t payload_size = 0;
    };

    struct Subscriber_stats {
        std::uint64_t cursor = 0;
        std::uint64_t dropped = 0;
        std::uint64_t owner_token = 0;
        std::uint64_t generation = 0;
        std::uint32_t state = 0;
    };

    struct Subscriber_metrics_snapshot {
        std::uint64_t heartbeat = 0;
        std::uint64_t last_seen_ns = 0;
        std::uint64_t pid = 0;
        std::uint64_t owner_token = 0;
        std::uint32_t generation = 0;
        std::uint32_t flags = 0;
    };

    Channel();
    explicit Channel(const Config& config);
    ~Channel();

    Channel(Channel&&) noexcept;
    Channel& operator=(Channel&&) noexcept;

    Channel(const Channel&)            = delete;
    Channel& operator=(const Channel&) = delete;

    class Reader;
    class Subscriber;
    class Chunk;

    [[nodiscard]] static constexpr std::size_t shared_alignment() noexcept
    {
        return SLOT_ALIGN;
    }

    [[nodiscard]] static constexpr bool valid_config(const Config& config) noexcept
    {
        return config.ring_size > 1 &&
               config.ring_size <= static_cast<std::size_t>(UINT32_MAX) &&
               config.subscriber_size > 0 && config.subscriber_size <= 63 &&
               config.payload_size <= static_cast<std::size_t>(UINT32_MAX);
    }

    [[nodiscard]] static constexpr std::size_t shared_size(
        const Config& config) noexcept
    {
        if (!valid_config(config))
            return 0;
        // 与 ipc/src/channel_impl.h 的 Header/Subscriber_state/Slot 布局一致
        constexpr std::size_t HEADER_SIZE = 11 * CACHE_LINE;
        constexpr std::size_t SUBSCRIBER_SIZE = CACHE_LINE;
        constexpr std::size_t SUBSCRIBER_METRICS_SIZE = CACHE_LINE;
        constexpr std::size_t SLOT_SIZE = CACHE_LINE;
        const std::size_t chunk_stride =
            (config.payload_size + SLOT_ALIGN - 1) & ~(SLOT_ALIGN - 1);
        return HEADER_SIZE + config.subscriber_size * SUBSCRIBER_SIZE +
               config.subscriber_size * SUBSCRIBER_METRICS_SIZE +
               config.ring_size * (SLOT_SIZE + chunk_stride);
    }

    [[nodiscard]] static Channel make(const Config& config);
    [[nodiscard]] static std::optional<Channel> create(void* region, std::size_t size,
                                                       const Config& config) noexcept;
    [[nodiscard]] static std::optional<Channel> attach(void* region,
                                                       std::size_t size) noexcept;

    [[nodiscard]] std::unique_ptr<Subscriber> subscribe(bool from_latest = false,
                                                        std::uint64_t owner_token = 0);
    [[nodiscard]] std::unique_ptr<Subscriber> subscribe_at(
        std::uint64_t position, std::uint64_t owner_token = 0);

    [[nodiscard]] bool publish(const Value& value,
                               std::uint64_t publish_time_ns = 0) noexcept;
    // chunk 模式：拷贝 payload 到固定 chunk 池，发布 chunk 索引。
    [[nodiscard]] bool publish_chunk(const void* data, std::size_t size,
                                     std::uint64_t publish_time_ns = 0) noexcept;

    // 先预留一个 chunk，调用方填写后再提交，避免一次额外拷贝。
    // Chunk 存活期间 Channel 不能 move/销毁；set_size 必须 <= capacity。
    [[nodiscard]] std::optional<Chunk> acquire_chunk();
    [[nodiscard]] bool publish(Chunk&& chunk) noexcept;

    [[nodiscard]] bool publisher_valid() const noexcept;
    [[nodiscard]] std::uint64_t publisher_token() const noexcept;
    [[nodiscard]] std::uint64_t publisher_generation() const noexcept;
    [[nodiscard]] std::uint64_t commit_pos() const noexcept;
    [[nodiscard]] std::uint64_t write_pos() const noexcept;
    [[nodiscard]] std::uint64_t last_published() const noexcept;
    [[nodiscard]] std::uint64_t last_publish_time_ns() const noexcept;
    [[nodiscard]] Stats stats() const noexcept;

    [[nodiscard]] bool begin_recovery(std::uint64_t token) noexcept;
    [[nodiscard]] bool end_recovery(std::uint64_t token) noexcept;
    [[nodiscard]] bool recover_subscriber(std::uint64_t owner_token,
                                          std::uint64_t token) noexcept;
    [[nodiscard]] bool recover_publisher(std::uint64_t token,
                                         std::uint64_t new_publisher_token) noexcept;

    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::size_t subscriber_size() const noexcept;
    [[nodiscard]] std::size_t payload_size() const noexcept;
    [[nodiscard]] std::uint64_t oldest_position() const noexcept;
    [[nodiscard]] std::uint64_t oldest_time_ns() const noexcept;
    [[nodiscard]] std::uint64_t publish_failures() const noexcept;

    // 预分配的 subscriber 观测接口；index 来自 Attach_response。
    [[nodiscard]] std::optional<Subscriber_stats> subscriber_stats(
        std::size_t index) const noexcept;
    [[nodiscard]] std::optional<Subscriber_metrics_snapshot> subscriber_metrics(
        std::size_t index) const noexcept;
    void update_subscriber_metrics(std::size_t index, std::uint64_t owner_token,
                                   std::uint64_t pid,
                                   std::uint64_t last_seen_ns) noexcept;

    // 返回第一个 publish_time >= time_ns 的逻辑位置；过旧返回最旧可用位置。
    [[nodiscard]] std::optional<std::uint64_t> position_at_time(
        std::uint64_t time_ns) const noexcept;

private:
    struct Impl;
    friend class Reader;
    friend class Subscriber;

    explicit Channel(std::unique_ptr<Impl> impl);

    [[nodiscard]] std::unique_ptr<Reader> make_reader(Subscriber& subscriber);
    void release_chunk(Chunk& chunk) noexcept;

    std::unique_ptr<Impl> impl_;
    bool chunk_outstanding_ = false;
};

class Channel::Chunk
{
public:
    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&& orig) noexcept;
    Chunk& operator=(Chunk&& orig) noexcept;

    [[nodiscard]] void* data() noexcept { return data_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    void set_size(std::size_t size);
    void set_publish_time_ns(std::uint64_t time_ns) noexcept;
    [[nodiscard]] std::uint64_t publish_time_ns() const noexcept;

private:
    friend class Channel;

    Chunk(Channel& owner, void* data, std::size_t capacity,
          std::uint64_t position) noexcept;

    Channel* owner_ = nullptr;
    void* data_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t size_ = 0;
    std::uint64_t position_ = 0;
    std::uint64_t publish_time_ns_ = 0;
    bool valid_ = false;
};

class Channel::Subscriber {
public:
    ~Subscriber();

    Subscriber(const Subscriber&)            = delete;
    Subscriber& operator=(const Subscriber&) = delete;
    Subscriber(Subscriber&&)                 = delete;
    Subscriber& operator=(Subscriber&&)      = delete;

    [[nodiscard]] std::unique_ptr<Reader> make_reader();
    void skip_to_latest() noexcept;
    [[nodiscard]] std::uint64_t cursor() const noexcept;
    [[nodiscard]] std::uint64_t dropped() const noexcept;

    // 录制/回放需要的只读查询与定位；seek 移动该 Subscriber 的共享游标。
    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::size_t payload_size() const noexcept;
    [[nodiscard]] std::uint64_t oldest_position() const noexcept;
    [[nodiscard]] std::uint64_t oldest_time_ns() const noexcept;
    [[nodiscard]] std::uint64_t publish_failures() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> position_at_time(
        std::uint64_t time_ns) const noexcept;
    [[nodiscard]] bool seek(std::uint64_t position) noexcept;

private:
    struct Impl;
    friend class Channel;

    Subscriber(Channel& parent, std::unique_ptr<Impl> impl);

    Channel* parent_ = nullptr;
    std::unique_ptr<Impl> impl_;
    bool chunk_outstanding_ = false;
};

class Channel::Reader {
public:
    ~Reader();

    Reader(const Reader&)            = delete;
    Reader& operator=(const Reader&) = delete;
    Reader(Reader&&)                 = delete;
    Reader& operator=(Reader&&)      = delete;

    [[nodiscard]] bool try_read() noexcept;
    [[nodiscard]] bool try_read_latest() noexcept;
    [[nodiscard]] bool try_read_nth_latest(std::uint64_t n) noexcept;
    [[nodiscard]] bool seek(std::uint64_t position) noexcept;
    void unpin() noexcept;
    [[nodiscard]] bool has_value() const noexcept;
    [[nodiscard]] const Value& value() const noexcept;
    [[nodiscard]] const void* payload() const noexcept;
    [[nodiscard]] std::size_t payload_size() const noexcept;
    [[nodiscard]] std::uint64_t publish_time_ns() const noexcept;

    template <typename Fn>
    [[nodiscard]] std::size_t try_read_recent(std::size_t max_n, Fn&& process) noexcept
    {
        static_assert(std::is_nothrow_invocable_v<Fn&, const Value&>,
                      "process must be noexcept");
        if (max_n == 0)
            return 0;
        Fn* fn = std::addressof(process);
        return read_recent(max_n, fn,
                           [](void* context, const Value& value) noexcept {
                               (*static_cast<Fn*>(context))(value);
                           });
    }

private:
    using Recent_callback = void (*)(void*, const Value&) noexcept;
    struct Impl;
    friend class Channel;

    explicit Reader(std::unique_ptr<Impl> impl);

    [[nodiscard]] std::size_t read_recent(std::size_t max_n, void* context,
                                          Recent_callback process) noexcept;

    std::unique_ptr<Impl> impl_;
    bool chunk_outstanding_ = false;
};

}  // namespace ipc
