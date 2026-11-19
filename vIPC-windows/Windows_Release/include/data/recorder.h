#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ipc/channel.h"
#include "data/export.h"
#include "data/recorder_config.h"
#include "data/trigger.h"

namespace data {

struct Queue_metrics
{
    std::uint64_t depth_bytes = 0;
    std::uint64_t depth_messages = 0;
    std::uint64_t high_watermark_bytes = 0;
    std::uint64_t overflow_count = 0;
};

struct Write_latency
{
    std::uint64_t p50_us = 0;
    std::uint64_t p95_us = 0;
    std::uint64_t p99_us = 0;
    std::uint64_t max_us = 0;
};

struct Bundle_metrics
{
    std::string name;
    Write_mode mode = Write_mode::SYNC;
    Queue_metrics queue;
    Write_latency latency;
    std::uint64_t enqueued = 0;
    std::uint64_t written = 0;
    std::uint64_t dropped = 0;
    std::uint64_t bytes_written = 0;
    std::uint64_t segment_count = 0;
    std::uint64_t unwritten = 0;
    std::uint64_t pre_roll_truncated = 0;
    std::uint64_t late_triggers = 0;
    std::uint64_t publish_failures = 0;
    bool mode_switch_pending = false;
    Write_mode pending_mode = Write_mode::SYNC;
};

// 一个待录制 topic：channel 由调用方创建并保持生命周期覆盖 Recorder。
struct Topic
{
    std::string name;
    ipc::Channel* channel = nullptr;

    std::string schema_name;
    std::string schema_encoding = "jsonschema";
    std::string schema_data = "{}";
    // 空表示按 channel 自动选择：chunk 模式 opaque，scalar 模式 uint64。
    std::string message_encoding;
};

// 已经 attach + subscribe 的 topic；由调用方（通常是 Broker）提供。
// Recorder 接管 subscriber，并自己创建 Reader。
struct Subscriber_topic
{
    std::string name;
    std::unique_ptr<ipc::Channel::Subscriber> subscriber;

    std::string schema_name;
    std::string schema_encoding = "jsonschema";
    std::string schema_data = "{}";
    std::string message_encoding;
};

struct Recorder_options
{
    std::string storage_directory = ".";
    std::size_t max_file_size = 100 * 1024 * 1024;
    // 按消息时间戳跨度切分；0 表示不按时长切分。
    std::chrono::milliseconds max_file_duration{0};
    std::string compression = "none";
    Write_mode write_mode = Write_mode::SYNC;
    std::size_t max_queue_bytes = 64 * 1024 * 1024;
    std::chrono::milliseconds poll_interval{100};

    // Supervisor 创建的 metrics 共享内存；空表示关闭。
    std::string metrics_shm_name;
    std::string metrics_region_name = "recorder_metrics";
    std::chrono::milliseconds metrics_interval{1000};

    // 一个 Trigger 完成（含 manifest）后回调；回调里可读取 files。
    std::function<void(const Trigger&)> on_trigger_done;
};

// 录制器：按 Trigger 的时间窗口，从 ipc Channel 读消息并写入 MCAP。
class LIBVALLEY_DATA_EXPORT Recorder
{
public:
    using New_file = std::function<void(const std::string& recorder,
                                        const std::string& bundle,
                                        std::size_t sequence,
                                        std::string& file_name)>;
    using Close_file = std::function<void(const std::string& file_name)>;

    Recorder();
    ~Recorder();

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    Recorder(Recorder&& orig) noexcept;
    Recorder& operator=(Recorder&& orig) noexcept;

    [[nodiscard]] static Recorder create(const std::string& name,
                                         const New_file& new_fn = {},
                                         const Close_file& close_fn = {});

    [[nodiscard]] static Recorder create(const std::string& name,
                                         const Recorder_options& options,
                                         const New_file& new_fn = {},
                                         const Close_file& close_fn = {});

    // 多 bundle 配置入口；配置非法时返回空 Recorder。
    [[nodiscard]] static Recorder create(const Recorder_config& config,
                                         const Recorder_options& options,
                                         const New_file& new_fn = {},
                                         const Close_file& close_fn = {});

    [[nodiscard]] bool add_topic(const Topic& topic);
    [[nodiscard]] bool add_topic(Subscriber_topic&& topic);

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    [[nodiscard]] bool failed() const noexcept;
    [[nodiscard]] std::string error() const;

    bool record(Trigger::Ptr&& trigger);
    void start();
    void stop();

    [[nodiscard]] std::vector<Bundle_metrics> metrics() const;
    [[nodiscard]] bool set_write_mode(const std::string& bundle_name,
                                      Write_mode mode);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace data
