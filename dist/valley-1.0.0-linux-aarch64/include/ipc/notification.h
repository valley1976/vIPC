// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 命名通知对象：内部共享内存状态由 Notification_state 实现。
// 每个 notification 绑定一个进程间 auto-reset event；create 必须提供名称。
// 外部只使用本句柄的 wait/set 接口，不依赖 Signals/Signals64。

#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <string>

namespace ipc {

class Notification
{
public:
    static constexpr std::size_t MAX_EVENTS = 256;

    Notification() = default;

    [[nodiscard]] static std::size_t required_size() noexcept;
    [[nodiscard]] static std::size_t required_alignment() noexcept;

    [[nodiscard]] static Notification create(
        void* region, std::size_t size,
        const std::string& event_name = {}) noexcept;
    [[nodiscard]] static Notification open(
        void* region, std::size_t size,
        const std::string& event_name = {}) noexcept;

    [[nodiscard]] bool valid() const noexcept;

    // 等待并消费一个事件（auto-reset）；超时/无效返回 false。
    [[nodiscard]] bool wait(std::size_t event,
                            std::chrono::milliseconds timeout) noexcept;
    [[nodiscard]] bool wait(std::size_t event) noexcept;

    // 等待任意事件并消费一个；适合 Workset 的 wait + poll。
    [[nodiscard]] bool wait_any(std::chrono::milliseconds timeout) noexcept;
    [[nodiscard]] bool wait_any() noexcept;

    // 返回被消费的事件下标；-1 表示超时/无效。
    [[nodiscard]] int wait_any_event(std::chrono::milliseconds timeout) noexcept;
    [[nodiscard]] int wait_any_event() noexcept;

    bool set(std::size_t event) noexcept;
    [[nodiscard]] bool empty() const noexcept;
    void reset() noexcept;

private:
    class Impl;
    explicit Notification(std::shared_ptr<Impl> impl) noexcept
        : impl_{std::move(impl)}
    {
    }

    std::shared_ptr<Impl> impl_;
};

}  // namespace ipc
