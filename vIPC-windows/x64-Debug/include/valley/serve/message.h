#pragma once

#include <cstring>
#include <string>

#include "valley/base/lang/storage.h"

#include "export.h"

namespace valley {
namespace serve {
namespace internal {
    class Message;
}

class LIBVALLEY_SERVE_EXPORT Message
{
public:
    Message();
    ~Message();

    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    explicit Message(internal::Message&& msg) noexcept;

    Message(Message&& orig) noexcept;
    Message& operator=(Message&& orig) noexcept;

    static Message create(size_t size);

    explicit operator bool() const;

    internal::Message& impl();

    void* data();
    const void* data() const;
    size_t size() const;

private:
    base::Storage<internal::Message, 8, 8> impl_;
};

inline Message make_message(const char* str) {
    auto len = std::strlen(str);
    auto msg = Message::create(len);
    std::strncpy(reinterpret_cast<char*>(msg.data()), str, len);

    return msg;
}

inline Message make_message(const std::string& str) {
    auto msg = Message::create(str.size());
    std::strncpy(reinterpret_cast<char*>(msg.data()), str.data(), str.size());

    return msg;
}

}
}