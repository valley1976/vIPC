#pragma once

#include <cstddef>

#include <valley/lang/storage.h>

namespace valley {
namespace thread {

class Option
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
    lang::Storage<Impl, 16, 8> impl_;
};

}
}