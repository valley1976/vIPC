#pragma once

#include <string>

#include "valley/base/export.h"

namespace valley {
namespace base {

int  LIBVALLEY_BASE_EXPORT get_last_error() noexcept;
void LIBVALLEY_BASE_EXPORT throw_system_error(const std::string& info);

}
}