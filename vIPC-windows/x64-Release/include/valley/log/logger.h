#pragma once

#include <cstdio>
#include <ostream>
#include <utility>

//#include <valley/api.h>

#ifdef vENABLE_LOG_FMT
  #define FMT_HEADER_ONLY 1
  #include <fmt/format.h>
#endif

#include "entry.h"
#include "context.h"

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
    Logger& operator<< (T&& val)
    {
        if (should_log_)
        {
            if (!entry_) {
                entry_.reset(new Entry(level_, line_, file_, func_));
                out_.reset(new OStream(entry_->payload, Entry::kMax_payload));
            }

            out_->out << std::forward<T>(val);
        }

        return *this;
    }

    template<typename ...Args>
    void printf(const char* fmt, Args&& ... args)
    {
        if (!should_log_)
            return;

        entry_.reset(new Entry(level_, line_, file_, func_));

#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wformat-security\"")
#endif

        entry_->payload_size = std::snprintf(entry_->payload,
            Entry::kMax_payload,
            fmt,
            std::forward<Args>(args)...);

        if (entry_->payload_size > Entry::kMax_payload)
        {   
            entry_->payload[Entry::kMax_payload - 3] = '.';
            entry_->payload[Entry::kMax_payload - 2] = '.';
            entry_->payload[Entry::kMax_payload - 1] = '.';

            entry_->payload_size = Entry::kMax_payload;
        }

#if defined(__GNUC__) || defined(__clang__)
        _Pragma("GCC diagnostic pop")
#endif
    }

#ifdef vENABLE_LOG_FMT
    template<typename F, typename ...Args>
    void format(F&& fmt, Args&& ... args)
    {
        if (!should_log_)
            return;

        entry_.reset(new Entry(level_, line_, file_, func_));

        entry_->payload_size = fmt::format_to_n(entry_->payload,
            Entry::kMax_payload,
            std::forward<F>(fmt),
            std::forward<Args>(args)...).size;

        if (entry_->payload_size > Entry::kMax_payload)
        {
            entry_->payload[Entry::kMax_payload - 3] = '.';
            entry_->payload[Entry::kMax_payload - 2] = '.';
            entry_->payload[Entry::kMax_payload - 1] = '.';

            entry_->payload_size = Entry::kMax_payload;
        }
    }
#endif

private:
    const bool should_log_;
    const Level level_;
    const int line_;
    const char* file_;
    const char* func_;

    Entry_ptr entry_;
    
    struct OStream {
        detail::Fixed_streambuf streambuf;
        std::ostream out;

        OStream(char* buffer, size_t size) : streambuf(buffer, size),
            out(&streambuf)
        {}
    };
    std::unique_ptr<OStream> out_;
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