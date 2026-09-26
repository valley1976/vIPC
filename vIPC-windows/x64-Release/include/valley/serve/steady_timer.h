#pragma once

#include <functional>
#include <memory>
#include <chrono>

#include "event_loop.h"

namespace valley {
namespace serve {

namespace internal {
class Steady_timer;
}

class LIBVALLEY_SERVE_EXPORT Steady_timer
{
public:
    explicit Steady_timer(Event_loop& ev);
    ~Steady_timer();

    Steady_timer(const Steady_timer&) = delete;
    Steady_timer& operator=(const Steady_timer&) = delete;

    Steady_timer(Steady_timer&&) = delete;
    Steady_timer& operator=(Steady_timer&&) = delete;

    void expire_after(const std::function<void(bool)>& action, std::chrono::milliseconds timespan);
    void expire_at(const std::function<void(bool)>& action, std::chrono::steady_clock::time_point tp);

    void cancel();

private:
    std::unique_ptr<internal::Steady_timer> impl_;
};

}
}