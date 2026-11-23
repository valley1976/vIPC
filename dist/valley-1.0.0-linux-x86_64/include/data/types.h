// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <chrono>
#include <cstddef>

#include "time/time.h"

namespace data {

using Clock = time_::Utc_clock;
using Time_point = time_::Utc_time;
using Steady_time = time_::Steady_time;
using Topic_id = std::size_t;

}  // namespace data
