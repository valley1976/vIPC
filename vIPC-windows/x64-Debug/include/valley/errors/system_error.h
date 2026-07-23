#pragma once

#include <string>

#include "valley/export/export.h"

namespace valley {
namespace errors {

void LIBVALLEY_EXPORT throw_system_error(const std::string& info);

}
}