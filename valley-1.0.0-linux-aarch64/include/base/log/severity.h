// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

namespace base {

enum class Log_severity : int
{
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

inline const char* level_string(Log_severity level)
{
    switch (level)
    {
    case Log_severity::TRACE: return "TRACE";
    case Log_severity::DEBUG: return "DEBUG";
    case Log_severity::INFO:  return "INFO";
    case Log_severity::WARN:  return "WARN";
    case Log_severity::ERROR: return "ERROR";
    case Log_severity::FATAL: return "FATAL";
    default:
        return "UNKNOWN";
    }
}

}
