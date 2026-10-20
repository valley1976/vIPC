#pragma once

#include <chrono>
#include <memory>
#include <functional>
#include <system_error>

#include "export.h"

namespace valley {
namespace serve {

namespace internal {
class Event_loop;
}

class LIBVALLEY_SERVE_EXPORT Event_loop
{
public:
    enum class Mode
    {
        kForeground, // 由调用者线程驱动
        kBackground, // 自起驱动线程
    };

    ///   触发线程：
    ///   bg+run  / bg+poll  : 后台驱动线程
    ///   fg+run             : 调用 start() 的线程（即 run() 所在线程）
    ///   fg+poll            : on_started 不触发；on_stopped 在 stop() 中触发；on_idle 不触发
    struct Event_handler
    {
        std::function<void()> on_started;   // 驱动循环进入前
        std::function<void()> on_idle;      // 仅 polling：每轮未处理到 handler 时
        std::function<void()> on_stopped;   // 驱动循环退出后
    };

    explicit Event_loop(const std::string& name, Event_handler eh = {}, Mode mode = Mode::kBackground);
    ~Event_loop();

    bool is_in_loop_thread() const;

    bool start(bool polling = false, bool background = true);
    bool stop();

    bool poll_one();

    size_t poll();
    size_t run_for(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    void dispatch(std::function<void()> fn);
    void post(std::function<void()> fn);

    internal::Event_loop& get_impl() { return *impl_; }

    static Event_loop& default_event_loop();

private:
    Event_loop(const std::shared_ptr<internal::Event_loop>& impl);

private:
    std::shared_ptr<internal::Event_loop> impl_;
};

}
}