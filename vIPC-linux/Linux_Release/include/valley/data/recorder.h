#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include "ipc/channel.h"
#include "valley/data/export.h"
#include "valley/data/trigger.h"

namespace valley {
namespace data {

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
    std::chrono::milliseconds poll_interval{100};
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

    [[nodiscard]] bool add_topic(const Topic& topic);
    [[nodiscard]] bool add_topic(Subscriber_topic&& topic);

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    [[nodiscard]] bool failed() const noexcept;
    [[nodiscard]] std::string error() const;

    void record(Trigger::Ptr&& trigger);
    void start();
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace data
}  // namespace valley
