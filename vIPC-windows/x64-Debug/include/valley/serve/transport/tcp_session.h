#pragma once

#include <memory>

#include <valley/base/lang/storage.h>
#include "valley/serve/core/types.h"
#include "valley/serve/core/message.h"

namespace valley {
namespace serve {

namespace internal {
class Tcp_session;
}

class LIBVALLEY_SERVE_EXPORT Tcp_session
{
public:
    explicit Tcp_session(internal::Tcp_session& impl);
    ~Tcp_session() = default;

    Tcp_session(const Tcp_session&) = delete;
    Tcp_session& operator=(const Tcp_session&) = delete;

    Tcp_session(Tcp_session&& orig) noexcept = delete;
    Tcp_session& operator=(Tcp_session&& orig) noexcept = delete;

    bool send_async(Message&& msg);

    bool receive_async(Message& msg, void* buffer, size_t size);
    bool receive_at_least_async(Message& msg, void* buffer, size_t size, size_t at_least);
    bool receive_exactly_async(Message& msg, void* buffer, size_t size, size_t exactly);

    void disconnect();

private:
    internal::Tcp_session& impl_;
};

}
}