#pragma once

#include <string>
#include <system_error>

#include "message.h"

namespace valley {
namespace local {

std::error_code make_error_code(Message::Status);

}
}

namespace std {
template<>
struct is_error_code_enum<valley::local::Message::Status> : true_type {};

}