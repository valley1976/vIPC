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

    Tcp_client(const Tcp_client&);
    Tcp_client& operator=(const Tcp_client&);

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

    using On_received = std::function<void(Message&, const void*, size_t)>;
    void set_on_received(const On_received& fn);

    bool connect();

    using On_connected = std::function<void()>;
    void connect_async(const On_connected& fn, bool auto_reconection = true);

    bool send_async(Message&& msg) const;

    bool receive_async(size_t size = 4 * 1024, size_t at_least = 0) const;
    bool receive_exactly_async(size_t size) const;

    bool disconnect() const;

private:
    std::shared_ptr<internal::Tcp_client> impl_;
};

}
}