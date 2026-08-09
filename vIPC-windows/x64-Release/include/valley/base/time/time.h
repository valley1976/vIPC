#pragma once

#include <cstddef>

#include "clock.h"

namespace valley {
namespace base {

using Timestamp_ns = uint64_t;

inline Timestamp_ns to_ns(const Clock::time_point& tp) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

inline Clock::time_point from_ns(Timestamp_ns ts) noexcept{
    return Clock::time_point{ std::chrono::nanoseconds(ts) };
}

}
}