#pragma once

#include <memory>

#include "valley/serve/core/context.h"
#include "valley/serve/core/bytes.h"

namespace valley {
namespace serve {

namespace tcp {
class Session;
class Client;
}

class LIBVALLEY_SERVE_EXPORT Tcp_client
{
public:
    Tcp_client();
    ~Tcp_client();

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

    using On_received = std::function<void(tcp::Session&, const Bytes_type&)>;
    void set_on_received(const On_received& fn);

    using On_connected = std::function<void(tcp::Session&)>;
    void set_on_connected(const On_connected& fn);

    void connect_async(bool auto_reconection = true);

    bool disconnect() const;

private:
    std::shared_ptr<tcp::Client> impl_;
};

}
}