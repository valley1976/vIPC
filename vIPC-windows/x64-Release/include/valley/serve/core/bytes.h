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

class LIBVALLEY_SERVE_EXPORT Bytes : public Bytes_type
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

private:
    class Bin;

    static size_t max_size_;
    Bin* bin_ = nullptr;
};

// inline 

inline size_t Bytes::max_size() { return max_size_; }

Bytes LIBVALLEY_SERVE_EXPORT make_bytes(const char* str);
Bytes LIBVALLEY_SERVE_EXPORT make_bytes(const std::string& str);

}
}