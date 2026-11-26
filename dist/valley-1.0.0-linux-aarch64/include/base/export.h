// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// base 共享库导出宏。
#if defined(_WIN32)
#  if defined(VALLEY_BASE_BUILD)
#    define LIBVALLEY_BASE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_BASE_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__)
#  define LIBVALLEY_BASE_EXPORT __attribute__((visibility("default")))
#else
#  define LIBVALLEY_BASE_EXPORT
#endif

#define VALLEY_BASE_EXPORT LIBVALLEY_BASE_EXPORT
