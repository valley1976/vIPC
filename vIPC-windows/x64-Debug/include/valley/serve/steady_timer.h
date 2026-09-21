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

class LIBVALLEY_SERVE_EXPORT Timer
{
public:
    explicit Timer(Event_loop& ev);
    ~Timer();

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void expire_after(const std::function<void(bool)>& action, std::chrono::milliseconds timespan);
    void expire_at(const std::function<void(bool)>& action, std::chrono::steady_clock::time_point tp);

    void cancel();

private:
    std::unique_ptr<internal::Steady_timer> impl_;
};

}
}