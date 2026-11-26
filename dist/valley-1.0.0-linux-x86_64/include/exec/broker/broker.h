// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "exec/export.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "ipc/channel.h"

namespace ipc {
class Event;
class Notification;
}

namespace exec {
namespace broker {

struct Message_view
{
    const void* data = nullptr;
    std::size_t size = 0;
    ipc::Channel::Value value = 0;
    std::uint64_t publish_time_ns = 0;
};

struct Broker_options
{
    std::string client_name;
    // 由 application 自动生成 supervisor 地址；addresses 非空时作为显式覆盖。
    std::string application;
    std::string socket_dir = "/tmp";
    std::vector<std::string> addresses;
};

class Broker;
class Publisher;
class Subscriber;

class LIBVALLEY_EXEC_EXPORT Publisher
{
public:
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] bool publish(const void* data, std::size_t size,
                               std::uint64_t publish_time_ns = 0) noexcept;
    [[nodiscard]] std::optional<ipc::Channel::Chunk> acquire_chunk();
    [[nodiscard]] bool publish(ipc::Channel::Chunk&& chunk) noexcept;

    // 发布成功后触发；必须在开始并发 publish 前设置。
    // wakeup 可以是 Event，也可以是任意回调；set_* 会替换现有集合。
    using Wakeup = std::function<void()>;
    [[nodiscard]] bool set_wakeup(std::shared_ptr<ipc::Event> event);
    void set_wakeups(std::vector<std::shared_ptr<ipc::Event>> events);
    void add_wakeup(Wakeup wakeup);

private:
    friend class Broker;
    Publisher(Broker& owner, std::string topic);

    Broker* owner_ = nullptr;
    std::string topic_;
    std::vector<Wakeup> wakeups_;
};

class LIBVALLEY_EXEC_EXPORT Subscriber
{
public:
    ~Subscriber();

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;

    [[nodiscard]] bool valid() const noexcept;

    void set_handler(std::function<void(const Message_view&)> handler);
    void start();
    void stop();

private:
    friend class Broker;
    Subscriber(Broker& owner, std::string topic);

    void run();

    Broker* owner_ = nullptr;
    std::string topic_;
    std::function<void(const Message_view&)> handler_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

// 每个客户进程一个实例；负责与 supervisor 的 attach、shm/Channel 缓存。
class LIBVALLEY_EXEC_EXPORT Broker
{
public:
    static Broker& instance();

    [[nodiscard]] bool start(const Broker_options& options);
    void stop();

    [[nodiscard]] std::shared_ptr<Publisher> acquire_publisher(
        const std::string& topic);
    [[nodiscard]] std::shared_ptr<Subscriber> acquire_subscriber(
        const std::string& topic);

    // pull 模式：返回由调用方持有的 Subscriber，适合 Recorder。
    // 不创建 Broker 本地读取线程；调用方必须在 Broker::stop() 前释放。
    [[nodiscard]] std::unique_ptr<ipc::Channel::Subscriber>
    acquire_subscriber_for_pull(const std::string& topic);

    // 附着 supervisor 暴露的命名 notification region。
    // 必须在至少一次 topic attach 之后调用。
    [[nodiscard]] std::shared_ptr<ipc::Notification>
    acquire_notification(const std::string& name);

    // 通过 Broker 查询 supervisor 暴露的 notification，再按其中的事件名表绑定 Event。
    [[nodiscard]] std::shared_ptr<ipc::Event>
    acquire_event(const std::string& name, const std::string& event_name);

    // 查询 supervisor：topic 属于哪些 recorder/bundle Notification。
    [[nodiscard]] std::vector<std::shared_ptr<ipc::Event>>
    acquire_wakeups(const std::string& topic);

private:
    Broker();
    ~Broker();

    Broker(const Broker&) = delete;
    Broker& operator=(const Broker&) = delete;

    friend class Publisher;
    friend class Subscriber;

    struct Impl;

    bool publish_chunk(const std::string& topic, const void* data,
                       std::size_t size, std::uint64_t publish_time_ns) noexcept;
    [[nodiscard]] std::optional<ipc::Channel::Chunk> acquire_chunk(
        const std::string& topic);
    bool publish_chunk(const std::string& topic,
                       ipc::Channel::Chunk&& chunk) noexcept;
    bool read_topic(const std::string& topic,
                    const std::function<void(const Message_view&)>& handler);
    void report_subscriber(const std::string& topic);
    [[nodiscard]] bool valid_topic(const std::string& topic) const;

    std::unique_ptr<Impl> impl_;
};

}  // namespace broker
}  // namespace exec
