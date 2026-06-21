#pragma once

#include <cstdio>
#include <ostream>
#include <utility>

//#include <valley/api.h>

#ifdef vENABLE_LOG_FMT
  #include <fmt/format.h>
#endif

#include "entry.h"

namespace valley {
namespace log {

namespace detail
{

class Fixed_streambuf : public std::streambuf
{
public:
    Fixed_streambuf(char* buffer, size_t size);

    Fixed_streambuf(const Fixed_streambuf&) = delete;
    Fixed_streambuf& operator=(const Fixed_streambuf&) = delete;

    std::size_t size() const;
    const char* c_str() const;

    void reset();

protected:
    int overflow(int ch) override;

private:
    char* data_;
    const size_t size_;
};

}

#ifdef _MSC_VER
#  pragma warning( disable: 4251 )
#endif

class Logger final
{
public:
    Logger(Level level, int line, const char* file, const char* func);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename T>
    std::ostream& operator<< (T&& val)
    {
        return out_ << std::forward<T>(val);
    }

    template<typename ...Args>
    void printf(const char* fmt, Args&& ... args)
    {
#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wformat-security\"")
#endif

        entry_->payload_size = std::snprintf(entry_->payload,
            Entry::kMax_payload,
            fmt,
            std::forward<Args>(args)...);

#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic pop")
#endif
    }

#ifdef vENABLE_LOG_FMT
    template<typename F, typename ...Args>
    void format(F&& fmt, Args&& ... args)
    {
        constexpr auto kMax_size = Entry::kMax_payload - 1;
        entry_->payload_size = fmt::format_to_n(entry_->payload,
            kMax_size,
            std::forward<F>(fmt),
            std::forward<Args>(args)...).size;

        entry_->payload[entry_->payload_size] = '\0';
    }
#endif

private:
    Entry_ptr entry_;
    detail::Fixed_streambuf streambuf_;
    std::ostream out_;
};

#ifdef _MSC_VER
#  pragma warning( default: 4251 )
#endif

class Null_logger
{
public:
    Null_logger(Level level, int line, const char* file)
    {};

    template<typename T>
    Null_logger& operator<< (T&& val)
    {
        return *this;
    }

    template<typename ...Args>
    void printf(const char* fmt, Args&& ... args)
    {}

    template<typename ...Args>
    void format(const char* fmt, Args&& ... args)
    {}
};

#ifndef VALLEY_ACTIVE_LOG_LEVEL
#define VALLEY_ACTIVE_LOG_LEVEL 0  
#endif

template<int Level, bool Enable = (Level >= VALLEY_ACTIVE_LOG_LEVEL)>
struct Use_logger
{
    using type = Logger;
};

template<int Level>
struct Use_logger<Level, false>
{
    using type = Null_logger;
};

}
}