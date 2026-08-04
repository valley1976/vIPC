#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>

namespace mcap {
using SchemaId = uint16_t;
using ChannelId = uint16_t;

struct McapWriterOptions;
}

namespace valley {
namespace data {

using Clock         = std::chrono::system_clock;
using Time_point    = std::chrono::system_clock::time_point;
using Topic_id      = size_t;

}
}