#pragma once

#include <cstddef>
#include <chrono>
#include <string>
#include <memory>
#include <vector>

#include "trigger.h"

#include "valley/data/export.h"

namespace valley {
namespace data {
namespace internal {
     class Recorder;
}

class LIBVALLEY_DATA_EXPORT Recorder
{
public:
    Recorder();
    ~Recorder();

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    Recorder(Recorder&& orig) noexcept;
    Recorder& operator=(Recorder&& orig) noexcept;

    static Recorder create(const std::string& name);

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    void record(Trigger::Ptr&& trigger);

    void start();
    void stop();

private:
    std::unique_ptr<internal::Recorder> impl_;
};

}
}