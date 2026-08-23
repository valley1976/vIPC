#pragma once

#include <memory>

#include "valley/serve/core/types.h"
#include "valley/serve/core/context.h"
#include "valley/serve/core/message.h"

namespace valley {
namespace serve {

namespace internal {
class Tcp_server;
}

class Tcp_session;

class LIBVALLEY_SERVE_EXPORT Tcp_server
{
public:
    Tcp_server();
    virtual ~Tcp_server();

    Tcp_server(const Tcp_server&);
    Tcp_server& operator=(const Tcp_server&);
    
    Tcp_server(Tcp_server&& orig) noexcept;
    Tcp_server& operator=(Tcp_server&& orig) noexcept;

    static Tcp_server create(
        const Context& context,
        uint16_t port, 
        const std::string& address = "");

    explicit operator bool() noexcept { return impl_ != nullptr; }

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;
    void setup_reuse_address(bool enable) noexcept;
    void setup_reuse_port(bool enable) noexcept;

    using On_connected = std::function<void(Tcp_session&)>;
    void set_on_connected(const On_connected& fn);

    using On_received = std::function<void(Tcp_session&, Message&, const void*, size_t)>;
    void set_on_received(const On_received& fn);

    virtual bool start();
    virtual void stop();

    virtual bool disconnect_all();

private:
    std::shared_ptr<internal::Tcp_server> impl_;
};

}
}