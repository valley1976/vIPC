#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <type_traits>

namespace valley {
namespace local {

class Buffer;

class Message
{
public:
    enum class Status : int32_t
    {
        kOk = 0,
        kCancelled = 1,
        kUnknown = 2,
        kInvalidArgument = 3,
        kDeadlineExceeded = 4,
        kNotFound = 5,
        kAlreadyExists = 6,
        kPermissionDenied = 7,
        kResourceExhausted = 8,
        kFailedPrecondition = 9,
        kAborted = 10,
        kOutOfRange = 11,
        kUnimplemented = 12,
        kInternal = 13,
        kUnavailable = 14,
        kDataLoss = 15,
        kUnauthenticated = 16,

        kUser = 100,
    };

    enum class Encoding : int16_t
    {
        kUnspec = 0,
        kString = 1,
        kJson = 2,
        kBinary = 3,
        kProtobuf = 4,
        kCapnp = 5,
    };

    struct Header;

public:
    explicit Message(size_t payload_size = 0);
    explicit Message(Buffer&& buffer);
    ~Message();

    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    Message(Message&& orig) noexcept;
    Message& operator=(Message&& orig) noexcept;

    Message& operator=(const std::string& msg);

    Header& header();
    const Header& header() const;

    void set_type(int16_t type);
    uint16_t type() const;

    void set_status(Status status);
    Status status() const;

    void set_encoding(Encoding encoding);
    Encoding encoding() const;

    void set_reference(uint64_t reference);
    uint64_t reference() const;

    bool valid() const;
    bool empty() const;

    void* payload();
    const void* payload() const;
    std::size_t payload_size() const;

    Buffer& buffer();
    const Buffer& buffer() const;

    void* resize_payload(size_t size);
    void* expand_payload(size_t extra);

    template<typename T>
    T* resize_payload()
    {
        static_assert(std::is_standard_layout<T>::value, "bad T");

        auto* ptr = resize_payload(sizeof(T));
        assert(std::uintptr_t(ptr) % alignof(T) == 0);

        return reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    T* cast_payload()
    {
        static_assert(std::is_standard_layout<T>::value, "bad T");
        assert(sizeof(T) <= payload_size() && std::uintptr_t(payload()) % alignof(T) == 0);
        return reinterpret_cast<T*>(payload());
    }

    template<typename T>
    const T* cast_payload() const
    {
        static_assert(std::is_standard_layout<T>::value, "bad T");
        assert(sizeof(T) <= payload_size() && std::uintptr_t(payload()) % alignof(T) == 0);
        return reinterpret_cast<const T*>(payload());
    }

    void reset();

    static const char* status_to_string(Status status);

    size_t byte_size() const;
    static size_t byte_size(size_t msg_len);

private:
    Header* get();
    const Header* get() const;

private:
    std::unique_ptr<Buffer> buffer_;
};

class Message_view
{
public:
    Message_view() = default;
    Message_view(const Message& msg);
    Message_view(const Buffer& buffer);
    Message_view(const void* ptr, size_t size);

    bool valid() const;

    const Message::Header& header() const { assert(valid()); return *header_; }

    uint16_t type() const;
    Message::Status status() const;
    Message::Encoding encoding() const;
    uint64_t reference() const;

    const void* payload() const;
    size_t payload_size() const;

    template<typename T>
    const T* cast_payload() const
    {
        static_assert(std::is_standard_layout<T>::value, "bad T");
        assert(sizeof(T) <= payload_size() && std::uintptr_t(payload()) % alignof(T) == 0);
        return reinterpret_cast<const T*>(payload());
    }

private:
    const Message::Header* header_ = nullptr;
};

void make_reply(const Message& request, Message& reply);
void make_reply(const Message_view& request, Message& reply);


// inline 

inline const char* Message::status_to_string(Status status)
{
    switch (status) {
    case Status::kOk:
        return "OK";
    case Status::kCancelled:
        return "CANCELLED";
    case Status::kUnknown:
        return "UNKNOWN";
    case Status::kInvalidArgument:
        return "INVALID_ARGUMENT";
    case Status::kDeadlineExceeded:
        return "DEADLINE_EXCEEDED";
    case Status::kNotFound:
        return "NOT_FOUND";
    case Status::kAlreadyExists:
        return "ALREADY_EXISTS";
    case Status::kPermissionDenied:
        return "PERMISSION_DENIED";
    case Status::kUnauthenticated:
        return "UNAUTHENTICATED";
    case Status::kResourceExhausted:
        return "RESOURCE_EXHAUSTED";
    case Status::kFailedPrecondition:
        return "FAILED_PRECONDITION";
    case Status::kAborted:
        return "ABORTED";
    case Status::kOutOfRange:
        return "OUT_OF_RANGE";
    case Status::kUnimplemented:
        return "UNIMPLEMENTED";
    case Status::kInternal:
        return "INTERNAL";
    case Status::kUnavailable:
        return "UNAVAILABLE";
    case Status::kDataLoss:
        return "DATA_LOSS";
    default:
        return "UNKNOWN_STATUS";
    }
}

}
}