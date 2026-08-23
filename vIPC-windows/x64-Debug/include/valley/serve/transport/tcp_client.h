#pragma once

#include <memory>

#include "valley/serve/core/types.h"
#include "valley/serve/core/context.h"
#include "valley/serve/core/message.h"

namespace valley {
namespace serve {

namespace internal {
class Tcp_client;
}

class LIBVALLEY_SERVE_EXPORT Tcp_client
{
public:
    Tcp_client();
    virtual ~Tcp_client();

    Tcp_client(const Tcp_client&) = delete;
    Tcp_client& operator=(const Tcp_client&) = delete;

    Tcp_client(Tcp_client&& orig) noexcept;
    Tcp_client& operator=(Tcp_client&& orig) noexcept;

    static Tcp_client create(
        const Context& context,
        const std::string& address,
        uint16_t port);

    explicit operator bool() noexcept { return impl_ != nullptr; }

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;

    bool is_connected();

    using On_connected = std::function<void()>;
    void set_on_connected(const On_connected& fn);

    using On_received = std::function<void(const void*, size_t)>;
    void set_on_received(const On_received& fn);

    bool connect();

    bool send_async(Message&& msg);

    bool receive_async(Message& msg, void* buffer, size_t size);
    bool receive_at_least_async(Message& msg, void* buffer, size_t size, size_t at_least);
    bool receive_exactly_async(Message& msg, void* buffer, size_t size, size_t exactly);

    virtual bool disconnect();

private:
    std::shared_ptr<internal::Tcp_client> impl_;
};

}
}