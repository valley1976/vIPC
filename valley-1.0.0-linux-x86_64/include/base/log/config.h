// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstddef>
#include <string>

#include "severity.h"

namespace base {

struct Log_config
{
    std::string directory = "logs";
    std::size_t max_size = 10 * 1024 * 1024;
    std::size_t max_files = 5;
    Log_severity level = Log_severity::INFO;
    bool console = false;
};

// 读取 log_config.json，然后应用 vLOG_* 环境变量。
// 环境变量优先级高于配置文件。
[[nodiscard]] Log_config load_log_config(const std::string& path = {});

}  // namespace base
