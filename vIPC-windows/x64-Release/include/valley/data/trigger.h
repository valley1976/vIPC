#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "types.h"

namespace valley {
namespace data {

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

class Trigger_manager {
public:
    ~Trigger_manager() = default;

    Trigger_manager(const Trigger_manager&) = delete;
    Trigger_manager& operator=(const Trigger_manager&) = delete;

    static void initialize_from_configuration();
    static int find_topic_id(const std::string& topic);
    static const std::vector<size_t>* find_tigger_topic(const std::string& trigger);

private:
    Trigger_manager() = default;
    void make_stock_trigger();

    static Trigger_manager& instance();

private:
    std::map<std::string, std::vector<size_t>> trigger_;
};

}
}