#pragma once

#include <ctime>
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

    Trigger(const std::string& name, const std::string& origin,
        std::chrono::milliseconds trigger_time_point, size_t before_sec, size_t after_sec);

    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;

    std::chrono::milliseconds trigger_time_ms() const {
        return base::time_point_to< std::chrono::milliseconds>(trigger_time);
    }

    size_t before_sec() const {
        auto diff = trigger_time - start_time;
        return std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    }

    size_t after_sec() const {
        auto diff = end_time - trigger_time;
        return std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    }

    using Ptr = std::unique_ptr<Trigger>;

    static Ptr make_unique(const std::string& name, const std::string& origin, 
        std::chrono::milliseconds trigger_time_point, size_t before_sec, size_t after_sec)
    {
        return Ptr(new Trigger(name, origin, trigger_time_point, before_sec, after_sec));
    }

    std::string to_string() const;
    static Ptr from_string(const std::string& str);

    const std::vector<size_t>* get_topic_id();

private:
    std::vector<size_t> topic_id_;
};

}
}