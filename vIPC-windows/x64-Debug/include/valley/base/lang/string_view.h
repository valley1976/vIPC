#pragma once

#include "internal/string_view.hpp"

namespace valley {
namespace base {

template<typename Char,  typename Traits = std::char_traits<Char>>
using Basic_string_view = bpstd::basic_string_view<Char, Traits>;

using String_view = Basic_string_view<char>;

}
}