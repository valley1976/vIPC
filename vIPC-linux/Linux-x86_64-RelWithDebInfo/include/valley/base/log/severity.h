#pragma once

namespace valley {
namespace base {

enum class Log_severity : int
{
    kTrace = 0,
    kDebug = 1,
    kInfo  = 2,
    kWarn  = 3,
    kError = 4,
    kFatal = 5
};

inline const char* level_string(Log_severity level)
{
    switch (level)
    {
    case Log_severity::kTrace: return "TRACE";
    case Log_severity::kDebug: return "DEBUG";
    case Log_severity::kInfo:  return "INFO";
    case Log_severity::kWarn:  return "WARN";
    case Log_severity::kError: return "ERROR";
    case Log_severity::kFatal: return "FATAL";
    default:
        return "UNKNOWN";
    }
}

}
}