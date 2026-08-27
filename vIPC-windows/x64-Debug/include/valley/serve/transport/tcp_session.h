#pragma once

#include <memory>

#include <valley/base/lang/storage.h>
#include "valley/serve/core/types.h"
#include "valley/serve/core/message.h"

namespace valley {
namespace serve {

namespace tcp {
class Session;
}

class LIBVALLEY_SERVE_EXPORT Tcp_session
{
public:
    explicit Tcp_session(tcp::Session& impl);
    ~Tcp_session() = default;

    Tcp_session(const Tcp_session&) = delete;
    Tcp_session& operator=(const Tcp_session&) = delete;

    Tcp_session(Tcp_session&& orig) noexcept = delete;
    Tcp_session& operator=(Tcp_session&& orig) noexcept = delete;

    bool send_async(Message&& msg);

    bool receive_async(size_t size = 4 * 1024, size_t at_least = 0);
    bool receive_exactly_async(size_t size);

    void disconnect();

private:
    tcp::Session& impl_;
};

}
}