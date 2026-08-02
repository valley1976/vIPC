#pragma once

#include <cstddef>
#include <memory>

#include "valley/base/export.h"
#include "valley/base/lang/storage.h"

namespace valley {
namespace base {
namespace internal {
    class Thread_option;
}

class LIBVALLEY_BASE_EXPORT Thread_option
{
public:
    Thread_option();
    ~Thread_option();

    void set_affinity(size_t cpu);

    void set_priority_idle();
    void set_priority_lowest();
    void set_priority_low();
    void set_priority_normal();
    void set_priority_high();
    void set_priority_highest();
    void set_priority_realtime();

    void apply() const;

private:
    std::shared_ptr<internal::Thread_option> impl_;
};

}
}