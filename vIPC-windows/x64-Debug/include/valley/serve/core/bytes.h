#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "valley/serve/export.h"

namespace valley {
namespace serve {

using Byte       = uint8_t;
using Bytes_type = std::vector<Byte>;

class LIBVALLEY_SERVE_EXPORT Bytes
{
public:
    Bytes();
    explicit Bytes(size_t size);
    ~Bytes();

    explicit Bytes(const Bytes_type& orig);
    explicit Bytes(Bytes_type&& orig);

    Bytes(const Bytes& other);
    Bytes& operator=(const Bytes& other);
    
    Bytes(Bytes&& orig) noexcept;
    Bytes& operator=(Bytes&& orig) noexcept;

    static void set_max_size(size_t kb);
    static size_t max_size();

    using iterator          = Bytes_type::iterator;
    using const_iterator    = Bytes_type::const_iterator;

    iterator begin() { return bytes_.begin(); }
    iterator end()   { return bytes_.end(); }

    const_iterator begin() const { return bytes_.begin(); }
    const_iterator end() const   { return bytes_.end(); }

    Byte& operator[](size_t pos)             { return bytes_[pos]; }
    const Byte& operator[](size_t pos) const { return bytes_[pos]; }

    Byte* data()             { return bytes_.data(); }
    const Byte* data() const { return bytes_.data(); }

    size_t capacity() const  { return bytes_.capacity(); }
    size_t size() const      { return bytes_.size(); }

    void reserve(size_t capacity) { bytes_.reserve(capacity); }
    void resize(size_t size)      { bytes_.resize(size); }

private:
    class Bin;

    static size_t max_size_;
    Bin* bin_ = nullptr;
    Bytes_type bytes_;
};

// inline 

inline size_t Bytes::max_size() { return max_size_; }

Bytes LIBVALLEY_SERVE_EXPORT make_bytes(const char* str);
Bytes LIBVALLEY_SERVE_EXPORT make_bytes(const std::string& str);

}
}