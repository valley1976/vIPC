// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// base 静态库当前不导出 DLL 符号；保留统一导出宏以兼容后续动态库构建。
#define LIBVALLEY_BASE_EXPORT
#define VALLEY_BASE_EXPORT LIBVALLEY_BASE_EXPORT
