// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// time 共享库导出宏。
#if defined(_WIN32)
#  if defined(VALLEY_TIME_BUILD)
#    define LIBVALLEY_TIME_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_TIME_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__)
#  define LIBVALLEY_TIME_EXPORT __attribute__((visibility("default")))
#else
#  define LIBVALLEY_TIME_EXPORT
#endif
