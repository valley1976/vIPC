#pragma once

#include <string>

#include "valley/base/export.h"

namespace valley {
namespace base {

void LIBVALLEY_BASE_EXPORT throw_system_error(const std::string& info);

}
}