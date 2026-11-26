// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include "base/export.h"

#include "context.h"
#include "record.h"

namespace base {

class LIBVALLEY_BASE_EXPORT Logger final
{
public:
    Logger(Log_severity level, int line, const char* file, const char* func,
           std::string tag = {});
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename T>
    Logger& operator<<(T&& value)
    {
        if (enabled_)
            stream_ << std::forward<T>(value);
        return *this;
    }

    void text(std::string message)
    {
        if (enabled_)
            stream_ << message;
    }

    template<typename ...Args>
    void printf(const char* format, Args... args)
    {
        if (!enabled_ || format == nullptr)
            return;

#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wformat-security\"")
#endif

        const int size = std::snprintf(nullptr, 0, format, args...);
        if (size > 0) {
            std::string text(static_cast<std::size_t>(size) + 1, '\0');
            std::snprintf(text.data(), text.size(), format, args...);
            text.resize(static_cast<std::size_t>(size));
            stream_ << text;
        }

#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic pop")
#endif
    }



private:
    bool enabled_;
    Log_record record_;
    std::ostringstream stream_;
};

class Null_logger
{
public:
    Null_logger(Log_severity level, int line, const char* file,
                const char* func, std::string tag = {})
    {
        (void)level;
        (void)line;
        (void)file;
        (void)func;
        (void)tag;
    }

    template<typename T>
    Null_logger& operator<<(T&&)
    {
        return *this;
    }

    void text(std::string)
    {
    }

    template<typename ...Args>
    void printf(const char*, Args&& ...)
    {
    }
};

#ifndef VALLEY_ACTIVE_LOG_LEVEL
#define VALLEY_ACTIVE_LOG_LEVEL 0
#endif

template<int Severity, bool Enable = (Severity >= VALLEY_ACTIVE_LOG_LEVEL)>
struct Use_logger
{
    using type = Logger;
};

template<int Severity>
struct Use_logger<Severity, false>
{
    using type = Null_logger;
};

}  // namespace base
