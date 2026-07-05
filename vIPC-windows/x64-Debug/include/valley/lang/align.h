#pragma once

#include <cassert>
#include <cstddef>

namespace valley {
namespace lang {

inline bool is_pow2(size_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

inline size_t align(size_t x, size_t a)
{
    assert(is_pow2(a));
    return ((x + a - 1) & ~(a - 1));
}

}
}