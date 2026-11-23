// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// vINFO_FMT 的格式化适配层；核心 log.h 不依赖 fmt/std::format。
#include <string>
#include <string_view>
#include <utility>

#include "log.h"

#if defined(VLOG_USE_STD_FORMAT)
#  if !defined(__cpp_lib_format)
#    error "VLOG_USE_STD_FORMAT requires C++20 <format> support"
#  endif
#  include <format>
#else
#  define FMT_HEADER_ONLY 1
#  include <fmt/format.h>
#endif

namespace base {

#if defined(VLOG_USE_STD_FORMAT)
template<typename Format, typename ...Args>
std::string format_text(Format&& format, Args&& ... args)
{
    return std::vformat(
        std::string_view{std::forward<Format>(format)},
        std::make_format_args(args...));
}
#else
template<typename Format, typename ...Args>
std::string format_text(Format&& format, Args&& ... args)
{
    return fmt::format(std::forward<Format>(format),
                       std::forward<Args>(args)...);
}
#endif

}  // namespace base

#define VLOG_FMT(severity, tag, ...) \
    VALLEY_LOG_TAG(severity, #tag).text(::base::format_text(__VA_ARGS__))

#define vTRACE_FMT(tag, ...)  VLOG_FMT(base::Log_severity::TRACE, tag, __VA_ARGS__)
#define vDEBUG_FMT(tag, ...)  VLOG_FMT(base::Log_severity::DEBUG, tag, __VA_ARGS__)
#define vINFO_FMT(tag, ...)   VLOG_FMT(base::Log_severity::INFO, tag, __VA_ARGS__)
#define vWARN_FMT(tag, ...)   VLOG_FMT(base::Log_severity::WARN, tag, __VA_ARGS__)
#define vERROR_FMT(tag, ...)  VLOG_FMT(base::Log_severity::ERROR, tag, __VA_ARGS__)
#define vFATAL_FMT(tag, ...)  VLOG_FMT(base::Log_severity::FATAL, tag, __VA_ARGS__)
