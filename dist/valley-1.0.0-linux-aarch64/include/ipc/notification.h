// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "ipc/export.h"

// 命名通知对象：内部共享内存状态由 Notification_state 实现。
// 每个 notification 绑定一个进程间 auto-reset event；create 必须提供 notification 名。
// 外部只使用本句柄的 poll/wait/set 接口，不依赖 Signals/Signals64。

#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <string>
#include <vector>

namespace ipc {

class LIBVALLEY_IPC_EXPORT Notification
{
public:
    static constexpr std::size_t MAX_EVENTS = 256;

    Notification() = default;

    [[nodiscard]] static std::size_t required_size() noexcept;
    [[nodiscard]] static std::size_t required_alignment() noexcept;

    [[nodiscard]] static Notification create(
        void* region, std::size_t size,
        const std::string& notification_name = {}) noexcept;
    [[nodiscard]] static Notification create(
        void* region, std::size_t size,
        const std::string& notification_name,
        const std::vector<std::string>& event_names) noexcept;
    [[nodiscard]] static Notification open(
        void* region, std::size_t size,
        const std::string& notification_name = {}) noexcept;

    [[nodiscard]] bool valid() const noexcept;

    // notification 名（共享内存中保存的名称）。
    [[nodiscard]] std::string name() const;

    [[nodiscard]] std::size_t event_count() const noexcept;
    [[nodiscard]] std::string event_name(std::size_t index) const;
    [[nodiscard]] int index_of(const std::string& event_name) const noexcept;

    // 初始化阶段写入事件名表；返回 false 表示名称非法/重复。
    [[nodiscard]] bool set_event_names(
        const std::vector<std::string>& event_names) noexcept;

    // Event::bind() 后冻结；冻结后 set_event_names() 返回 false。
    void freeze_event_names() noexcept;

    // 非阻塞消费一个事件；没有 pending 事件返回 -1。
    [[nodiscard]] int poll() noexcept;

    // 阻塞直到至少有一个事件出现；不消费事件。
    // 超时/无效返回 false。
    [[nodiscard]] bool wait() noexcept;
    [[nodiscard]] bool wait(std::chrono::milliseconds timeout) noexcept;

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
