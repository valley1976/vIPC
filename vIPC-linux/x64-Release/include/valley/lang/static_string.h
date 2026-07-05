#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdarg>

namespace valley {
namespace lang {

template<size_t N>
class Static_string
{
public:
    Static_string() 
    {
        data_[0] = '\0';
    }

    const char* c_str() const { return data_; }
    size_t length() const { return length_; }

    void assign(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        auto n = std::vsnprintf(data_, N, fmt, args);
        va_end(args);

        if (0 <= n && n <= N)
            length_ = n;
        else
            clear();
    }

    void clear()
    {
        length_ = 0;
        data_[0] = '\0';
    }

    bool operator==(const char* str) const
    {
        return std::strcmp(data_, str) == 0;
    }

    bool operator!=(const char* str) const
    {
        return std::strcmp(data_, str) != 0;
    }

    template<size_t M>
    bool operator==(const Static_string<M>& str) const
    {
        return std::strcmp(data_, str.c_str()) == 0;
    }

    template<size_t M>
    bool operator!=(const Static_string<M>& str) const
    {
        return std::strcmp(data_, str.c_str()) != 0;
    }

private:
    size_t length_ = 0;
    char data_[N];
};

template<size_t N>
using SStr = Static_string<N>;

}
}