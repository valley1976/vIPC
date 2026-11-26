// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// storage_mcap 共享库导出宏。
#if defined(_WIN32)
#  if defined(VALLEY_STORAGE_MCAP_BUILD)
#    define LIBVALLEY_STORAGE_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_STORAGE_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__)
#  define LIBVALLEY_STORAGE_EXPORT __attribute__((visibility("default")))
#else
#  define LIBVALLEY_STORAGE_EXPORT
#endif
