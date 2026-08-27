#pragma once

#include "internal/string_view.hpp"

namespace valley {
namespace base {

template<typename Char,  typename Traits = std::char_traits<Char>>
using Basic_string_view = bpstd::basic_string_view<Char, Traits>;

using String_view = Basic_string_view<char>;
using WString_view = Basic_string_view<wchar_t>;
using U16String_view = Basic_string_view<char16_t>;
using U32String_view = Basic_string_view<char32_t>;

}
}