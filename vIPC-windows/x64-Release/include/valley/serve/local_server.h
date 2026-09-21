#pragma once

#include <memory>
#include <string>
#include <system_error>

#include "event_loop.h"

namespace valley {
namespace serve {

namespace internal {
class Local_server;
}

class LIBVALLEY_SERVE_EXPORT Local_server
{
public:
    Local_server(Event_loop& event_loop, const std::string& address);

    const std::string& path() const noexcept;

    uint64_t connected_sessions() const noexcept;

    bool option_keep_alive() const noexcept;
    bool option_no_delay() const noexcept ;
    bool option_reuse_address() const noexcept;
    bool option_reuse_port() const noexcept;

    //! Is the server started?
    bool is_started() const noexcept;

    bool start();
    bool stop();
    bool restart();

    bool multicast(const void* buffer, size_t size);

    bool disconnect_all();

    //std::shared_ptr<session_type> find_session(uint64_t id);

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;
    void setup_reuse_address(bool enable) noexcept;
    void setup_reuse_port(bool enable) noexcept;

    using Session = void*;

    struct LIBVALLEY_SERVE_EXPORT Handler
    {
        std::function<void(Session)> on_connected;
        std::function<void(Session)> on_disconnected;

        std::function<void(Session, const void*/*buffer*/, size_t/*size*/, size_t&/*consumed, default is same size*/)> on_received;
        std::function<void(Session, size_t/*sent*/, size_t/*pending*/)> on_sent;
        std::function<void(Session)> on_empty;

        std::function<void(Session, const std::error_code&)> on_error;

        static uint64_t get_id(Session s);
        static bool send_async(Session s, const void* buffer, size_t size);
        static bool disconnect_async(Session s);
    };

    void set_handler(std::unique_ptr<Handler>& h);

    std::shared_ptr<internal::Local_server> get_impl() { return impl_; }

private:
    std::shared_ptr<internal::Local_server> impl_;
};

}
}