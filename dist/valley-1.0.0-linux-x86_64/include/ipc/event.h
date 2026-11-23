// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 对外事件句柄：隐藏 Notification 的共享内存布局和位图实现。
//
//   ipc::Event::bind("FAULT", region, size, {"安全带未系", "相机被遮挡"});
//   ipc::Event cam("FAULT", "相机被遮挡");
//   cam.emit();
//
// bind 负责把共享内存 region 附着为 Notification；Event 只保存组、事件下标
// 和通知对象的引用，不暴露实现类型。

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ipc {

class Notification;

class Event
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

    // 在共享内存 region 上创建/附着 notification，并登记事件顺序。
    // region 生命周期必须覆盖所有使用该 group 的 Event。
    [[nodiscard]] static bool bind(const std::string& group, void* region,
                                   std::size_t size,
                                   const std::vector<std::string>& events);

    // 使用已有的 Notification 句柄登记事件顺序，不需要调用方持有 region。
    [[nodiscard]] static bool bind(const std::string& group,
                                   Notification& notification,
                                   const std::vector<std::string>& events);

    static void unbind(const std::string& group) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ipc
