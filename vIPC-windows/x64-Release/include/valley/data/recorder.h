#pragma once

#include <cstddef>
#include <chrono>
#include <string>
#include <memory>
#include <vector>

#include "data/trigger.h"

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

    void record(Trigger::Ptr&& trigger);

    void start();
    void stop();

private:
    std::unique_ptr<internal::Recorder> impl_;
};

}
}