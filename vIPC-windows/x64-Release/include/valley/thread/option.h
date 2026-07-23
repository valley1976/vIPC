#pragma once

#include <cstddef>
#include <memory>

#include "valley/export/export.h"
#include "valley/lang/storage.h"

namespace valley {
namespace thread {

class LIBVALLEY_EXPORT Option
{
public:
    Option();
    ~Option();

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
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}