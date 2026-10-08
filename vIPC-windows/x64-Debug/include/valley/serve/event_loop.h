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
    explicit Event_loop(const std::string& name);
    ~Event_loop();

    bool is_in_loop_thread() const;

    bool start(bool polling = false, bool background = true);
    bool stop();

    bool poll_one();
    bool run_one();

    size_t poll();
    size_t run_for(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    void dispatch(std::function<void()> fn);
    void post(std::function<void()> fn);

    struct Event_handler
    {
        std::function<void()> on_thread_initialize;
        std::function<void()> on_started;
        std::function<void()> on_idle;
        std::function<void()> on_stopped;
        std::function<void()> on_thread_cleanup;
        std::function<void(const std::error_code&)> on_error;
    };

    bool set_handler(std::unique_ptr<Event_handler>& handler);

    internal::Event_loop& get_impl() { return *impl_; }

    static Event_loop& default_event_loop();

private:
    Event_loop(const std::shared_ptr<internal::Event_loop>& impl);

private:
    std::shared_ptr<internal::Event_loop> impl_;
};

}
}