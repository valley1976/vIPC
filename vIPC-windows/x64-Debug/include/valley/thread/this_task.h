#pragma once

#include <cstddef>

#include "valley/export/export.h"

namespace valley {
namespace thread {

class LIBVALLEY_EXPORT This_task
{
public:
    static void schedule() noexcept;
};

}
}