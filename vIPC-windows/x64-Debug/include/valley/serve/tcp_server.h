#pragma once

#include <memory>
#include <string>
#include <system_error>

#include "valley/base/lang/any.h"

#include "event_loop.h"

namespace valley {
namespace serve {

namespace internal {
class Tcp_server;

template<typename Handler>
class Tcp_session;
}

class LIBVALLEY_SERVE_EXPORT Tcp_server
{
public:
    Tcp_server(Event_loop& event_loop, int port, bool is_ipv4 = true);
    Tcp_server(Event_loop& event_loop, const std::string& address, int port);

    const std::string& address() const noexcept;
    int port() const noexcept;

    uint64_t connected_sessions() const noexcept;

    bool is_keep_alive() const noexcept;
    bool is_no_delay() const noexcept ;
    bool is_reuse_address() const noexcept;
    bool is_reuse_port() const noexcept;

    //! Is the server started?
    bool is_started() const noexcept;

    bool start();
    bool stop();
    bool restart();

    bool broadcast(const void* buffer, size_t size);

    bool disconnect_all();

    //std::shared_ptr<session_type> find_session(uint64_t id);

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;
    void setup_reuse_address(bool enable) noexcept;
    void setup_reuse_port(bool enable) noexcept;

    struct LIBVALLEY_SERVE_EXPORT Session
    {
        using session_type = internal::Tcp_session<internal::Tcp_server>;
        using Ptr = std::shared_ptr<session_type>;

        static uint64_t get_id(const Ptr& ses);
        static base::Any& get_user_data(const Ptr& ses);
        // copy buffer then send async
        static bool send_async(const Ptr& ses, const void* buffer, size_t size);
        static bool disconnect_async(const Ptr& ses);
    };

    struct LIBVALLEY_SERVE_EXPORT Handler
    {
        std::function<void(const Session::Ptr&)> on_connected;
        std::function<void(const Session::Ptr&)> on_disconnected;

        std::function<void(const Session::Ptr&, const void*/*buffer*/, size_t/*size*/, size_t&/*consumed, default is same size*/)> on_received;
        std::function<void(const Session::Ptr&, size_t/*sent*/, size_t/*pending*/)> on_sent;
        std::function<void(const Session::Ptr&)> on_empty;

        std::function<void(const Session::Ptr&, const std::error_code&)> on_error;
    };

    bool set_handler(std::unique_ptr<Handler>&& h);

private:
    std::shared_ptr<internal::Tcp_server> get_impl() { return impl_; }

private:
    std::shared_ptr<internal::Tcp_server> impl_;
};

}
}