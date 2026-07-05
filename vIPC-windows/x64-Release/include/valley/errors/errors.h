#pragma once

#include <iostream>
#include <functional>

#include "system_error.h"

namespace valley {
namespace errors {

void install_proccess_failure_handler(const std::string& exe_file = "", const std::string& dump_path = "coredump");

void trace();
void trace(std::ostream& os);

}
}