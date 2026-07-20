#pragma once

#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace mcap {
using SchemaId = uint16_t;
using ChannelId = uint16_t;

struct McapWriterOptions;
}

namespace valley {
namespace data {

using Time_point    = std::chrono::high_resolution_clock::time_point;
using Topic_id      = size_t;

struct Trigger
{
    const std::string name;
    const std::string origin;

    const Time_point trigger_time; 
    const Time_point start_time;
    const Time_point end_time;

    std::string description;
    std::vector<Topic_id> topic;

    Trigger(const std::string& name, const std::string& origin, size_t before_sec, size_t after_sec) : name(name),
        origin(origin),
        trigger_time(std::chrono::high_resolution_clock::now()),
        start_time(trigger_time - std::chrono::seconds(before_sec)),
        end_time(trigger_time + std::chrono::seconds(after_sec))
    {}

    using Ptr = std::unique_ptr<Trigger>;

    static Ptr make_unique(const std::string& name, const std::string& origin, size_t before_sec, size_t after_sec)
    {
        return Ptr(new Trigger(name, origin, before_sec, after_sec));
    }
};

}
}