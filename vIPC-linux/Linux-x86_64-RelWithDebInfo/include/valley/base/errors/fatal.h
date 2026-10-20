#pragma once

#include <cstdlib>
#include <string>
#include <stdexcept>
#include <system_error>

#include "system_error.h"

#include "valley/base/export.h"

namespace valley {
namespace base {

void LIBVALLEY_BASE_EXPORT fatal(const char* file, int line, const char* func, const std::string& message, int error = get_last_error()); 
void LIBVALLEY_BASE_EXPORT fatal(const char* file, int line, const char* func, const std::exception& e);
void LIBVALLEY_BASE_EXPORT fatal(const char* file, int line, const char* func, const std::error_code& ec);
void LIBVALLEY_BASE_EXPORT fatal(const char* file, int line, const char* func, const std::system_error& e);

}
}

#define fatality(...)               valley::base::fatal(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define fatality_system_error(msg)  valley::base::fatal(__FILE__, __LINE__, __func__, std::system_error(base::get_last_error(), std::system_category(), msg))
