#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include <type_traits>

#include "valley/base/lang/aligned_allocator.h"

#include "protocol.h"

namespace valley {
namespace ipc {
namespace serve {

class Frame
{
public:
    using Byte = Protocol::u8;

    // 与线上布局严格一致，无 padding
    struct Header {
        Protocol::u32 seq;
        Protocol::u16 method;
        Protocol::u16 payload_len;
        Protocol::u32 status;
        Protocol::u8  version;
        Protocol::u8  type;
        Protocol::u8  enc;
        Protocol::u8  flags;
    };

    static constexpr auto kFRAME_HEADER_SIZE = sizeof(Header);
    static_assert(kFRAME_HEADER_SIZE == 16, "Frame_header size must match kFRAME_HEADER_SIZE");

    static constexpr auto kMaxPayload = 64 * 1024 * 1024 - kFRAME_HEADER_SIZE;

    Frame() = default;
    ~Frame() = default;

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    bool empty() const;

    const Header* header() const;

    void reserve(size_t total);

    Byte* resize_payload(Protocol::u16 size);

    Byte* payload();
    size_t payload_size();

    const Byte* payload() const;
    size_t payload_size() const;

    const Byte* data() const;
    size_t size() const;

    void make_request(Protocol::u16 method_id, Protocol::u32 seq);
    void make_response(Protocol::u16 method_id, Protocol::u32 seq, Protocol::u32 status);
    void make_notification(Protocol::u16 method_id, Protocol::u32 seq);

    template<typename T>
    T* make_payload();
    template<typename T>
    T* make_request(Protocol::u16 method_id, Protocol::u32 seq);
    template<typename T>
    T* make_response(Protocol::u16 method_id, Protocol::u32 seq, Protocol::u32 status = 0);
    template<typename T>
    T* make_notification(Protocol::u16 method_id, Protocol::u32 seq);

    static const Header* parse_header(const void* buffer, size_t size);

private:
    using Bytes = std::vector<Byte, base::Aligned_allocator<Byte, 64>>;

    Header* header();

private:
    Bytes bytes_;
};

// inline

inline bool Frame::empty() const
{
    return bytes_.empty();
}

inline Frame::Header* Frame::header()
{ 
    assert(!bytes_.empty());
    return reinterpret_cast<Header*>(bytes_.data());
}

inline const Frame::Header* Frame::header() const
{
    assert(!bytes_.empty());
    return reinterpret_cast<const Header*>(bytes_.data());
}


inline void Frame::reserve(size_t total)
{
    bytes_.reserve(total);
}

inline Frame::Byte* Frame::resize_payload(Protocol::u16 size)
{
    assert(size <= kMaxPayload);

    bytes_.resize(kFRAME_HEADER_SIZE + size);

    auto* h = header();
    std::memset(h, 0, sizeof(Header));
    h->version = Protocol::kPROTOCOL_VERSION;
    h->payload_len = size;

    return size >0 ? bytes_.data() + kFRAME_HEADER_SIZE : nullptr;
}

template<typename T>
inline T* Frame::make_payload()
{
    constexpr bool is_memcpy_safe = 
        std::is_trivially_copyable<T>::value
        && std::is_standard_layout<T>::value;

    static_assert(is_memcpy_safe, "T must be trivially copyable and standard layout");
    static_assert(kFRAME_HEADER_SIZE % alignof(T) == 0, "T alignment incompatible with frame header size");
    static_assert(sizeof(T) <= kMaxPayload, "T exceeds maximum payload size");

    return reinterpret_cast<T*>(resize_payload(sizeof(T)));
}

inline Frame::Byte* Frame::payload() { return empty() ? nullptr : bytes_.data() + kFRAME_HEADER_SIZE; }
inline size_t Frame::payload_size() { return empty() ? 0 : bytes_.size() - kFRAME_HEADER_SIZE; }

inline const Frame::Byte* Frame::payload() const { return empty() ? nullptr : bytes_.data() + kFRAME_HEADER_SIZE; }
inline size_t Frame::payload_size() const { return empty() ? 0 : bytes_.size() - kFRAME_HEADER_SIZE; }

inline const Frame::Byte* Frame::data() const { return bytes_.data(); }
inline size_t Frame::size() const { return bytes_.size(); }

inline void Frame::make_request(Protocol::u16 method_id, Protocol::u32 seq)
{
    resize_payload(0);

    auto* h = header();

    h->type = Protocol::u8(Protocol::Type::kREQUEST);
    h->method = method_id;
    h->seq = seq;
}
inline void Frame::make_response(Protocol::u16 method_id, Protocol::u32 seq, Protocol::u32 status)
{
    resize_payload(0);

    auto* h = header();

    h->type = Protocol::u8(Protocol::Type::kRESPONSE);
    h->method = method_id;
    h->seq = seq;
    h->status = status;
}

inline void Frame::make_notification(Protocol::u16 method_id, Protocol::u32 seq)
{
    resize_payload(0);

    auto* h = header();

    h->type = Protocol::u8(Protocol::Type::kNOTIFICATION);
    h->method = method_id;
    h->seq = seq;
}

template<typename T>
inline T* Frame::make_request(Protocol::u16 method, Protocol::u32 seq)
{
    auto* payload = make_payload<T>();
    if (payload)
    {
        auto* h = header();
        h->type = Protocol::u8(Protocol::Type::kREQUEST);
        h->method = method;
        h->seq = seq;
    }

    return payload;
}

template<typename T>
inline T* Frame::make_response(Protocol::u16 method_id, Protocol::u32 seq, Protocol::u32 status)
{
    auto* payload = make_payload<T>();

    if (payload)
    {
        auto* h = header();

        h->type = Protocol::u8(Protocol::Type::kRESPONSE);
        h->method = method_id;
        h->seq = seq;
        h->status = status;
    }

    return payload;
}

template<typename T>
inline T* Frame::make_notification(Protocol::u16 method_id, Protocol::u32 seq)
{
    auto* payload = make_payload<T>();

    if (payload)
    {
        auto* h = header();

        h->type = Protocol::u8(Protocol::Type::kNOTIFICATION);
        h->method = method_id;
        h->seq = seq;
    }

    return payload;
}

inline const Frame::Header* Frame::parse_header(const void* buffer, size_t size)
{
    if (size >= kFRAME_HEADER_SIZE)
        return reinterpret_cast<const Header*>(buffer);

    return nullptr;
}

}
}
}