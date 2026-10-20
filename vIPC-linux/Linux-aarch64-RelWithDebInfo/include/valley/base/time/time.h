#pragma once

#include <cstddef>

#include "clock.h"

namespace valley {
namespace base {

template<typename Duration = std::chrono::milliseconds>
inline Duration time_point_to(const Clock::time_point& pt) noexcept
{
    return std::chrono::duration_cast<Duration>(pt.time_since_epoch());
}

template<typename Duration = std::chrono::milliseconds>
inline Clock::time_point time_point_from(Duration d) noexcept
{
    return Clock::time_point{ d };
}

inline std::chrono::milliseconds now_ms() noexcept
{
    return time_point_to<std::chrono::milliseconds>(Clock::now());
}

}
}