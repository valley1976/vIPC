#pragma once

#include <cstdint>
#include <chrono>
#include <atomic>
#include <thread>

#include "valley/base/export.h"
#include "valley/base/log/log.h"

#include "option.h"
#include "this_task.h"
#include "workset.h"

namespace valley {
namespace base {

uint64_t current_thread_id();

using Timepoint = std::chrono::high_resolution_clock::time_point;

inline Timepoint now() { return std::chrono::high_resolution_clock::now(); }

bool LIBVALLEY_BASE_EXPORT nanosleep_until(Timepoint next_tp);

template <class Fn, class... Args>
inline std::thread start(const Thread_option& option, Fn&& fn, Args&&... args)
{
    return std::thread([option, fn, args...]()
    {
        option.apply();

        // Call the base thread function
        fn(std::move(args)...);
    });
}

template <class Fn, class... Args>
inline std::thread start_cyclic(const Thread_option& option,
    std::atomic<bool>& stop,
    const Timepoint& start, const std::chrono::nanoseconds& period,
    Fn&& fn, Args&&... args)
{
    return std::thread([option,
        &stop,
        start, period,
        fn, args...]()
    {
        option.apply();

        Timepoint next_tp = start;
        nanosleep_until(next_tp);

        while(!stop.load(std::memory_order_acquire))
        {
            fn(std::move(args)...);

            next_tp += period;
            nanosleep_until(next_tp);
        }
    });
}

}
}
