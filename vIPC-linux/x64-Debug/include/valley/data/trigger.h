#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "export.h"
#include "types.h"

namespace valley {
namespace data {

class LIBVALLEY_DATA_EXPORT Trigger
{
public:
    const std::string name;
    const std::string origin;

    const Time_point trigger_time;
    const Time_point start_time;
    const Time_point end_time;

    uint64_t sequence; // reserve
    std::string description;
    std::vector<std::string> topic;

    Trigger(const std::string& name, const std::string& origin, size_t before_sec, size_t after_sec);

    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;

    using Ptr = std::unique_ptr<Trigger>;

    static Ptr make_unique(const std::string& name, const std::string& origin, size_t before_sec, size_t after_sec)
    {
        return Ptr(new Trigger(name, origin, before_sec, after_sec));
    }

    const std::vector<size_t>* get_topic_id();

private:
    std::vector<size_t> topic_id_;
};

}
}