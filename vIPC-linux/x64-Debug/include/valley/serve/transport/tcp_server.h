#pragma once

#include <memory>

#include "valley/serve/core/context.h"
#include "valley/serve/core/message.h"

namespace valley {
namespace serve {

namespace tcp {
class Session;
class Server;
}

class LIBVALLEY_SERVE_EXPORT Tcp_server
{
public:
    Tcp_server();
    ~Tcp_server();

    Tcp_server(const Tcp_server&);
    Tcp_server& operator=(const Tcp_server&);
    
    Tcp_server(Tcp_server&& orig) noexcept;
    Tcp_server& operator=(Tcp_server&& orig) noexcept;

    static Tcp_server create(const Context& context, uint16_t port) { return create(context, "", port); }
    static Tcp_server create(const Context& context, const std::string& address, uint16_t port);

    explicit operator bool() noexcept { return impl_ != nullptr; }

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;
    void setup_reuse_address(bool enable) noexcept;
    void setup_reuse_port(bool enable) noexcept;

    using On_connected = std::function<void(tcp::Session&)>;
    void set_on_connected(const On_connected& fn);

    using On_received = std::function<void(tcp::Session&, Message&, const void*, size_t)>;
    void set_on_received(const On_received& fn);

    bool start_accept();
    void stop();

    bool disconnect_all();

private:
    std::shared_ptr<tcp::Server> impl_;
};

}
}