#pragma once

#include <memory>
#include <string>
#include <system_error>

#include "export.h"
#include "event_loop.h"

namespace valley {
namespace serve {

namespace internal {
class Tcp_client;
}


class LIBVALLEY_SERVE_EXPORT Tcp_client
{
public:
    Tcp_client(Event_loop& event_loop, const std::string& address, int port);

    const std::string& address() const noexcept;
    int port() const noexcept;

    //! Get the option: keep alive
    bool option_keep_alive() const noexcept;
    //! Get the option: no delay
    bool option_no_delay() const noexcept;

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;

    bool connect_async();
    bool disconnect_async();
    bool reconnect_async();

    bool send_async(const void* buffer, size_t size);

    struct Handler
    {
        std::function<void()> on_connected;
        std::function<void()> on_disconnected;

        std::function<void(const void*, size_t, size_t&)> on_received;
        std::function<void(size_t, size_t)> on_sent;
        std::function<void()> on_empty;

        std::function<void(const std::error_code&)> on_error;
    };

    void set_handler(std::unique_ptr<Handler>& handler);

private:
    std::shared_ptr<internal::Tcp_client> impl_;
};

}
}