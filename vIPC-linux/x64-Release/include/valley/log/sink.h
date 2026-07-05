#pragma once

#include <memory>
#include <vector>

#include "entry.h"

namespace valley {
namespace log {

struct Sink
{
    virtual ~Sink() = default;
    virtual void write(Entry_ptr& entry) = 0;
};

using Sink_ptr = std::unique_ptr<Sink>;
using Sink_group = std::vector<Sink_ptr>;

Sink_ptr make_std_sink();

}
}