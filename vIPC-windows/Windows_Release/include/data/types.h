#pragma once

#include <chrono>
#include <cstddef>

namespace valley {
namespace data {

using Clock = std::chrono::system_clock;
using Time_point = Clock::time_point;
using Topic_id = std::size_t;

}  // namespace data
}  // namespace valley
