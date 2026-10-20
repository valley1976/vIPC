#pragma once

#include "internal/tl/expected.hpp"

namespace valley {
namespace base {

template<typename T, typename E>
using Expected = tl::expected<T, E>;

template<typename E>
inline auto make_unexpected(E && e) -> tl::unexpected<typename std::decay<E>::type>
{
    return tl::make_unexpected(std::forward<E>(e));
}

}
}