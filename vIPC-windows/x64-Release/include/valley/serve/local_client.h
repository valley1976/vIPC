#pragma once

#include <memory>
#include <string>
#include <system_error>

#include "export.h"
#include "event_loop.h"

namespace valley {
namespace serve {

namespace internal {
class Local_client;
}


class LIBVALLEY_SERVE_EXPORT Local_client
{
public:
    Local_client(Event_loop& event_loop, const std::string& address);

    const std::string& path() const noexcept;

    //! Get the option: keep alive
    bool option_keep_alive() const noexcept;

    void setup_keep_alive(bool enable) noexcept;

    bool connect_async();
    bool disconnect_async();
    bool reconnect_async();

    bool send_async(const void* buffer, size_t size);

    struct Handler
    {
        std::function<void()> on_connected;
        std::function<void()> on_disconnected;

        std::function<void(const void*/*buffer*/, size_t/*size*/, size_t&/*consumed, default is same size*/)> on_received;
        std::function<void(size_t/*sent*/, size_t/*pending*/)> on_sent;
        std::function<void()> on_empty;

        std::function<void(const std::error_code&)> on_error;
    };

    void set_handler(std::unique_ptr<Handler>& handler);

private:
    std::shared_ptr<internal::Local_client> impl_;
};

}
}