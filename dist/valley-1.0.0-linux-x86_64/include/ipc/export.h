// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// ipc 共享库导出宏。
#if defined(_WIN32)
#  if defined(VALLEY_IPC_BUILD)
#    define LIBVALLEY_IPC_EXPORT __declspec(dllexport)
#  else
#    define LIBVALLEY_IPC_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__)
#  define LIBVALLEY_IPC_EXPORT __attribute__((visibility("default")))
#else
#  define LIBVALLEY_IPC_EXPORT
#endif
