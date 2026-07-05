#pragma once

namespace valley {
namespace log {

enum class Level : int
{
    kTrace = 0,
    kDebug = 1,
    kInfo  = 2,
    kWarn  = 3,
    kError = 4,
    kFatal = 5
};

inline const char* level_string(Level level)
{
    switch (level)
    {
    case Level::kTrace: return "TRACE";
    case Level::kDebug: return "DEBUG";
    case Level::kInfo:  return "INFO";
    case Level::kWarn:  return "WARN";
    case Level::kError: return "ERROR";
    case Level::kFatal: return "FATAL";
    default:
        return "UNKNOWN";
    }
}

}
}