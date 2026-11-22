// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace base {

template<std::uint32_t N>
class Static_string
{
    static_assert(N > 0, "Static_string capacity must be > 0");

public:
    Static_string()
    {
        data_[0] = '\0';
    }

    [[nodiscard]] bool empty() const { return length_ == 0; }
    [[nodiscard]] const char* c_str() const { return data_; }
    [[nodiscard]] std::size_t length() const { return length_; }

    void assign(const char* str)
    {
        if (str == nullptr) {
            clear();
            return;
        }

        std::size_t len = std::strlen(str);
        if (len > N - 1)
            len = N - 1;
        std::memcpy(data_, str, len);
        data_[len] = '\0';
        length_ = static_cast<std::uint32_t>(len);
    }

    void printf(const char* format, ...)
    {
        if (format == nullptr) {
            clear();
            return;
        }

        va_list args;
        va_start(args, format);
        const int written = std::vsnprintf(data_, N, format, args);
        va_end(args);

        if (written < 0) {
            clear();
            return;
        }

        if (static_cast<std::size_t>(written) >= N) {
            data_[N - 1] = '\0';
            length_ = static_cast<std::uint32_t>(N - 1);
        } else {
            length_ = static_cast<std::uint32_t>(written);
        }
    }

    void clear()
    {
        length_ = 0;
        data_[0] = '\0';
    }

    bool operator==(const char* str) const
    {
        return str != nullptr && std::strcmp(data_, str) == 0;
    }

    bool operator!=(const char* str) const
    {
        return !(*this == str);
    }

    template<std::uint32_t M>
    bool operator==(const Static_string<M>& str) const
    {
        return std::strcmp(data_, str.c_str()) == 0;
    }

    template<std::uint32_t M>
    bool operator!=(const Static_string<M>& str) const
    {
        return !(*this == str);
    }

private:
    std::uint32_t length_ = 0;
    char data_[N]{};
};

template<std::uint32_t N>
using SStr = Static_string<N>;

}  // namespace base
