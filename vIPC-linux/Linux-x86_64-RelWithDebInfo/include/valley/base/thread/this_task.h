#pragma once

#include <cstddef>

#include "valley/base/export.h"

namespace valley {
namespace base {

class LIBVALLEY_BASE_EXPORT This_task
{
public:
    static void schedule() noexcept;
};

}
}