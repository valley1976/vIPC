#pragma once

#include <cstdint>
#include <memory>

#include "valley/base/export.h"

namespace valley {
namespace base {

namespace internal {
class Event;
}

class LIBVALLEY_BASE_EXPORT Event
{
public:
    Event();
    ~Event();

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    bool set(uint32_t value);
    uint32_t get();
    
    void wait();
    bool wait(int millisencds);

    size_t max_signal() const;

private:
    std::unique_ptr<internal::Event> impl_;
};

}
}