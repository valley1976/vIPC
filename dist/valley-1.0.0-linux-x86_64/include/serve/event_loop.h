// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "serve/export.h"

// 事件循环：asio::io_context 的薄封装。
//   background 模式：start() 起线程后立刻返回，stop() 停线程；
//   foreground 模式：start() 在当前线程阻塞运行，等价 run()。
// 所有回调都在 loop 线程上执行；跨线程投递用 post()/dispatch()。
// 约定：poll()/poll_one()/run_for() 只用于前台手动驱动，不能与 start() 混用。
// 公开头文件不暴露 asio；需要 io_context 的内部代码用 detail/event_loop_internal.h。

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace serve {

namespace detail {
struct Event_loop_internal;
}

class LIBVALLEY_SERVE_EXPORT Event_loop
{
public:
    enum class Mode
    {
        FOREGROUND,
        BACKGROUND
    };

    explicit Event_loop(std::string name = "loop", Mode mode = Mode::BACKGROUND);
    ~Event_loop();

    Event_loop(const Event_loop&)            = delete;
    Event_loop& operator=(const Event_loop&) = delete;
    Event_loop(Event_loop&&)                 = delete;
    Event_loop& operator=(Event_loop&&)      = delete;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }
    [[nodiscard]] bool is_in_loop_thread() const noexcept;

    // background 模式起线程后返回；foreground 模式阻塞到 stop()
    void start();
    // 停止并回收线程；不要在 loop 线程里销毁 Event_loop
    void stop();
    // stop() 之后再次驱动，需要显式 restart()
    void restart();

    [[nodiscard]] std::size_t poll();
    [[nodiscard]] bool poll_one();
    [[nodiscard]] std::size_t run_for(std::chrono::milliseconds timeout);

    void dispatch(std::function<void()> fn);
    void post(std::function<void()> fn);

    // 进程内共享的后台循环，首次调用时启动
    [[nodiscard]] static Event_loop& default_loop();

private:
    friend struct detail::Event_loop_internal;

    struct Impl;

    void run();

    std::unique_ptr<Impl> impl_;
    std::string name_;
    Mode mode_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<std::thread::id> loop_thread_{std::thread::id{}};
};

}  // namespace serve
