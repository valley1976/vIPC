#pragma once

#include <cstddef>
#include <cstdint>

#include "valley/base/time/time.h" 

namespace mcap {
using SchemaId = uint16_t;
using ChannelId = uint16_t;

struct McapWriterOptions;
}

namespace valley {
namespace data {

using Clock         = base::Clock;
using Time_point    = base::Clock::time_point;
using Topic_id      = size_t;

}
}