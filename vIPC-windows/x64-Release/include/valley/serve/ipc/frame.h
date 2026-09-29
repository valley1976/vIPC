#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <type_traits>

#include "valley/base/lang/aligned_allocator.h"
#include "valley/serve/export.h"

#include "types.h"

namespace valley {
namespace serve {
namespace ipc {

struct Frame_header;

class LIBVALLEY_SERVE_EXPORT Frame
{
public:
    ~Frame() = default;

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    Frame(Frame&& orig) noexcept;
    Frame& operator=(Frame&& orig) noexcept;

    bool empty() const;

    u8 method() const;
    u32 sequence() const;
    
    Byte* payload();
    u16 payload_size();

    const Byte* payload() const;
    u16 payload_size() const;

    const Byte* data() const;
    u16 size() const;

    void set_sequence(u32 seq);

protected:
    Frame() = default;

    Frame_header* header();
    const Frame_header* header() const;

protected:
    using Bytes = std::vector<Byte, base::Aligned_allocator<Byte, 64>>;

    Bytes bytes_;
};

template<typename T>
T* get(Frame& frame)
{
    constexpr bool is_memcpy_safe =
        std::is_trivially_copyable<T>::value
        && std::is_standard_layout<T>::value;

    static_assert(is_memcpy_safe, "T must be trivially copyable and standard layout");
    static_assert(16 % alignof(T) == 0, "T alignment incompatible with frame header size");

    return reinterpret_cast<T*>(frame.payload());
}

// inline

inline Frame::Frame(Frame&& orig) noexcept : bytes_(std::move(orig.bytes_))
{}

inline Frame& Frame::operator=(Frame&& orig) noexcept
{
    if (this != &orig)
        bytes_ = std::move(orig.bytes_);

    return *this;
}

inline bool Frame::empty() const
{
    return bytes_.empty();
}

//--------------------------------------------------

class Request : public Frame
{
public:
    Request() = default;

    void make_request(u16 method, u16 payload_size);
};

class Response : public Frame
{
public:
    Response() = default;

    u32 status() const;

    void make_response(u16 method, u32 sequence, u32 status);
    void make_response(u16 method, u32 sequence, u32 status, u16 payload_size);
};

class Notification : public Frame
{
public:
    Notification() = default;
    
    void make_notification(u16 method, u16 payload_size);
};

template<typename T>
const T* cast_as(const Frame& frame)
{
    constexpr bool is_memcpy_safe =
        std::is_trivially_copyable<T>::value
        && std::is_standard_layout<T>::value;

    static_assert(is_memcpy_safe, "T must be trivially copyable and standard layout");
    static_assert(16 % alignof(T) == 0, "T alignment incompatible with frame header size");

    return reinterpret_cast<const T*>(frame.payload());
}

}
}
}