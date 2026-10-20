#pragma once

#include "internal/any.h"

namespace valley {
namespace base {

using Any = libany::any;
using libany::any_cast;
using libany::swap;
using libany::bad_any_cast;

}
}