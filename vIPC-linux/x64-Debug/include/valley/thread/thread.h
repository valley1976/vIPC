#pragma once

#ifdef _WIN32
#include <Windows.h>
#undef Yield
#undef max
#undef min
#else
#include <time.h>
#endif // !_WIN32

#include <cstdint>
#include <chrono>
#include <atomic>
#include <thread>

#include "option.h"
#include "this_task.h"
#include "workset.h"

namespace valley {
namespace thread {

uint64_t current_thread_id();

struct Timepoint
{
#ifdef _WIN32
    LARGE_INTEGER value;
#else
    struct timespec value;
#endif

    Timepoint after(const std::chrono::seconds& sec) const;
    Timepoint after(const std::chrono::nanoseconds& ns) const;
};

Timepoint now();
void nanosleep_until(const Timepoint& tp);

template <class Fn, class... Args>
inline std::thread start(const Option& option, Fn&& fn, Args&&... args)
{
    return std::thread([&option, fn, args...]()
    {
        option.apply();

        // Call the base thread function
        fn(std::move(args)...);
    });
}

template <class Fn, class... Args>
inline std::thread start_cyclic(const Option& option,
    std::atomic<bool>& stop,
    const Timepoint& start, const std::chrono::nanoseconds& period,
    Fn&& fn, Args&&... args)
{
    return std::thread([&option,
        &stop,
        start, period,
        fn, args...]()
    {
        option.apply();

        Timepoint next_period = start;
        nanosleep_until(next_period);

        while(!stop.load(std::memory_order_acquire))
        {
            fn(std::move(args)...);

            next_period = next_period.after(period);
            nanosleep_until(next_period);
        }
    });
}

}
}
