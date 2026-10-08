#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include "valley/base/lang/any.h"
#include "valley/serve/ipc/frame.h"

#include "event_loop.h"

namespace valley {
namespace serve {

namespace ipc {

class Server;

template<typename Handler>
class Stream;

using session_type  = ipc::Stream<ipc::Server>;
using Session_ptr   = std::shared_ptr<session_type>;

using On_response   = std::function<void(const Session_ptr&, const Response_view)>;

uint64_t get_id(const Session_ptr& ses);
base::Any& get_user_data(const Session_ptr& ses);
std::error_code request_async(const Session_ptr& ses, const Request& request, const On_response& h, const Busy_option& option = Busy_policy::kRejectIfBusy);
std::error_code notify_async(const Session_ptr& ses, const Notification& n);
bool disconnect_async(const Session_ptr& ses);

};

class LIBVALLEY_SERVE_EXPORT Ipc_server
{
public:
    using On_connected          = std::function<void(const ipc::Session_ptr&)>;
    using On_disconnected       = std::function<void(const ipc::Session_ptr&)>;

    using Method_handler        = std::function<void(const ipc::Session_ptr&, const ipc::Request_view, ipc::Response&)>;
    using Notification_handler  = std::function<void(const ipc::Session_ptr&, const ipc::Notification_view)>;

public:
    explicit Ipc_server(const std::string& address);
    Ipc_server(Event_loop& event_loop, const std::string& address);
    ~Ipc_server() noexcept = default;

    Ipc_server(const Ipc_server&) = delete;
    Ipc_server& operator=(const Ipc_server&) = delete;

    Ipc_server(Ipc_server&&) noexcept;
    Ipc_server& operator=(Ipc_server&&) noexcept;

    const std::string& path() const noexcept;

    uint64_t connected_sessions() const noexcept;

    bool set_method_max_alive_time(ipc::Ms time);

    bool set_on_connected(const On_connected& h);
    bool set_on_disconnected(const On_disconnected& h);
    
    //! Is the server started?
    bool is_started() const noexcept;

    bool start();
    bool stop();

    bool disconnect_all();

    ipc::Session_ptr find_session(uint64_t id);

    bool register_method(uint16_t method_id, const Method_handler& h);
    bool register_notification(uint16_t method_id, const Notification_handler& h);

    std::error_code request_async(uint64_t session_id, const ipc::Request& request, const ipc::On_response& h, ipc::Busy_policy policy = ipc::Busy_policy::kRejectIfBusy);
    std::error_code notify_async(uint64_t session_id, const ipc::Notification& n);
    //bool broadcast_notify(const ipc::Notification& n);

private:
    std::shared_ptr<ipc::Server> get_impl() { return impl_; }

private:
    std::shared_ptr<ipc::Server> impl_;
};

inline Ipc_server::Ipc_server(Ipc_server&& orig) noexcept : impl_(std::move(orig.impl_))
{}

inline Ipc_server& Ipc_server::operator=(Ipc_server&& orig) noexcept
{
    if(this != &orig)
        impl_ = std::move(orig.impl_);

    return *this;
}

}
}