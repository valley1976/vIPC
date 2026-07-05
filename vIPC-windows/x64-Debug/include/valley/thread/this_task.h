#pragma once

#include <cstddef>

#include <valley/lang/storage.h>

namespace valley {
namespace thread {

class This_task
{
public:
    static void schedule() noexcept;
};

}
}