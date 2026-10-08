#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include "valley/base/lang/any.h"
#include "valley/serve/ipc/frame.h"

#include "export.h"
#include "event_loop.h"

namespace valley {
namespace serve {

namespace ipc {
class Client;
}

class LIBVALLEY_SERVE_EXPORT Ipc_client
{
public:
    using On_connected          = std::function<void()>;
    using On_disconnected       = std::function<void()>;

    using Method_handler        = std::function<void(const ipc::Request_view, ipc::Response&) >;
    using Notification_handler  = std::function<void(const ipc::Notification_view)>;
    using On_response           = std::function<void(const ipc::Response_view)>;

public:
    explicit Ipc_client(const std::string& address);
    Ipc_client(Event_loop& event_loop, const std::string& address);
    ~Ipc_client() noexcept = default;

    Ipc_client(const Ipc_client&) = delete;
    Ipc_client& operator=(const Ipc_client&) = delete;

    Ipc_client(Ipc_client&&) noexcept;
    Ipc_client& operator=(Ipc_client&&) noexcept;

    const std::string& path() const noexcept;

    base::Any& user_data() noexcept;

    bool set_method_max_alive_time(ipc::Ms time);

    bool set_on_connected(const On_connected& h);
    bool set_on_disconnected(const On_disconnected& h);

    bool connect_async();
    bool disconnect_async();
    bool reconnect_async();

    bool register_method(uint16_t method_id, const Method_handler& h);
    bool register_notification(uint16_t method_id, const Notification_handler& h);

    std::error_code request_async(const ipc::Request& r, const On_response& h, const ipc::Busy_option& option = ipc::Busy_policy::kRejectIfBusy);
    std::error_code notify_async(const ipc::Notification& n);

private:
    std::shared_ptr<ipc::Client> impl_;
};

inline Ipc_client::Ipc_client(Ipc_client&& orig) noexcept : impl_(std::move(orig.impl_))
{}

inline Ipc_client& Ipc_client::operator=(Ipc_client&& orig) noexcept
{
    if (this != &orig)
        impl_ = std::move(orig.impl_);
    
    return *this;
}

}
}