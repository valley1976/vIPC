#pragma once

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstring>

namespace valley {
namespace base {

template<uint32_t N>
class Static_string
{
public:
    Static_string() 
    {
        data_[0] = '\0';
    }

    const char* c_str() const { return data_; }
    size_t length() const { return length_; }

    void assign(const char* str) {
        assert(str);
        auto len = std::strlen(str);
        if (len > N-1)
            len = N-1;
        std::strncpy(data_, str, len);
        data_[len] = '\0';
    }

    void printf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        auto n = std::vsnprintf(data_, N, fmt, args);
        va_end(args);

        if (0 < n)
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

    template<uint32_t M>
    bool operator==(const Static_string<M>& str) const
    {
        return std::strcmp(data_, str.c_str()) == 0;
    }

    template<uint32_t M>
    bool operator!=(const Static_string<M>& str) const
    {
        return std::strcmp(data_, str.c_str()) != 0;
    }

private:
    uint32_t length_ = 0;
    char data_[N];
};

template<uint32_t N>
using SStr = Static_string<N>;

}
}