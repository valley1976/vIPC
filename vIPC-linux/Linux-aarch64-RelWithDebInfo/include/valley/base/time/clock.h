#pragma once

#include <cstdint>
#include <chrono>
#include <functional>

#include "valley/base/export.h"

namespace valley {
namespace base {

// 时钟的纪元（Epoch）是 Unix 纪元（1970-01-01 00:00:00 UTC）
struct LIBVALLEY_BASE_EXPORT Clock
{
    using rep               = std::int64_t;
    using period            = std::nano;  // 纳秒级精度
    using duration          = std::chrono::duration<rep, period>;
    using time_point        = std::chrono::time_point<Clock>;

    static constexpr bool is_steady = false;

    template<typename Duration>
    static
        std::chrono::time_point<std::chrono::system_clock, Duration>
        to_sys(std::chrono::time_point<Clock, Duration> const& tp)
    {
        return std::chrono::time_point<std::chrono::system_clock, Duration>(tp.time_since_epoch());
    }

    template<typename Duration>
    static
        std::chrono::time_point<Clock, Duration>
        from_sys(std::chrono::time_point<std::chrono::system_clock, Duration> const& tp)
    {
        return std::chrono::time_point<Clock, Duration>(tp.time_since_epoch());
    }

    static time_point now() noexcept;

    using Now_impl = std::function<time_point()>;
    static void set_now_impl(const Now_impl& impl);
};

}
}