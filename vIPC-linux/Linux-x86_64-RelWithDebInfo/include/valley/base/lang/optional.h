#pragma once

#include "internal/tl/optional.hpp"

namespace valley {
namespace base {

using tl::nullopt;

template<typename T>
using Optional = tl::optional<T>;

}
}