#pragma once

// 静态库默认无导出属性；保留宏以便以后切换为 shared。
#if defined(_WIN32) && defined(VALLEY_STORAGE_BUILD_SHARED)
#if defined(VALLEY_STORAGE_MCAP_BUILD)
#define LIBVALLEY_STORAGE_EXPORT __declspec(dllexport)
#else
#define LIBVALLEY_STORAGE_EXPORT __declspec(dllimport)
#endif
#else
#define LIBVALLEY_STORAGE_EXPORT
#endif
