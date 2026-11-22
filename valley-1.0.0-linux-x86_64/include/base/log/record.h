// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "severity.h"

namespace base {

// 一条已采集的日志记录；格式化由 Sink 完成。
struct Log_record
{
    Log_severity severity = Log_severity::INFO;
    std::chrono::system_clock::time_point timestamp{};
    std::uint64_t process_id = 0;
    std::uint64_t thread_id = 0;
    std::string file;
    std::string function;
    int line = 0;
    std::string tag;
    std::string message;
};

}  // namespace base
