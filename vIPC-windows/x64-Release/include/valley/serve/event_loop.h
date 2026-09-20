#pragma once

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

    bool start(bool polling = false);
    bool stop();
    bool restart();

    void dispatch(std::function<void()> fn);
    void post(std::function<void()> fn);

    internal::Event_loop& get_impl() { return *impl_; }

    struct Handler
    {
        std::function<void()> on_thread_initialize;
        std::function<void()> on_started;
        std::function<void()> on_idle;
        std::function<void()> on_stopped;
        std::function<void()> on_thread_cleanup;
        std::function<void(const std::error_code&)> on_error;
    };

    bool set_handler(std::unique_ptr<Handler>& handler);

private:
    std::unique_ptr<internal::Event_loop> impl_;
};

}
}