// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "ipc/export.h"

// 对外事件句柄：隐藏 Notification 的共享内存布局和位图实现。
//
//   auto notification = ipc::Notification::create(region, size, "FAULT", events);
//   ipc::Event::bind("FAULT", notification);
//   ipc::Event cam("FAULT", "相机被遮挡");
//   cam.emit();
//
// bind 负责把共享内存 region 附着为 Notification；Event 只保存组、事件下标
// 和通知对象的引用，不暴露实现类型。

#include <memory>
#include <string>

namespace ipc {

class Notification;

class LIBVALLEY_IPC_EXPORT Event
{
public:
    Event(std::string group, std::string event_name);
    ~Event();

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&& orig) noexcept;
    Event& operator=(Event&& orig) noexcept;

    [[nodiscard]] bool valid() const noexcept;

    // 置位对应事件；无效 Event 静默忽略。
    void emit() noexcept;

    // 附着已有 Notification；事件名表从 Notification 读取。
    [[nodiscard]] static bool bind(const std::string& group,
                                   Notification& notification);

    static void unbind(const std::string& group) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ipc
