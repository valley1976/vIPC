#pragma once

#include <cstddef>
#include <chrono>
#include <string>
#include <memory>
#include <vector>

#include "data/types.h"

namespace valley {
namespace data {
namespace internal {
     class Recorder;
}

class Recorder
{
public:
    explicit Recorder(const std::string& name);
    ~Recorder();

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    void initialize();

    void record(Trigger::Ptr&& trigger);

    void start();
    void stop();

private:
    bool set_storage_path(const std::string& path);
    bool set_topic_subscriber(const std::string& name);

private:
    std::unique_ptr<internal::Recorder> impl_;
};

}
}