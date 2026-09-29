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
    using Method_handler = std::function<void(uint16_t/*method*/, uint32_t/*sequence*/, const void*/*payload*/, uint16_t/*payload_size*/, ipc::Response&/*response*/) >;
    using Notification_handler = std::function<void(uint16_t/*method*/, uint32_t/*sequence*/, const void*/*payload*/, uint16_t/*payload_size*/)>;
    using On_response = std::function<void(uint16_t/*method*/, uint32_t/*sequence*/, uint32_t/*status*/, const void*/*payload*/, uint16_t/*payload_size*/)>;

public:
    Ipc_client(Event_loop& event_loop, const std::string& address);

    const std::string& path() const noexcept;

    base::Any& user_data() noexcept;

    //! Get the option: keep alive
    bool option_keep_alive() const noexcept;

    void setup_keep_alive(bool enable) noexcept;

    bool connect_async();
    bool disconnect_async();
    bool reconnect_async();

    bool register_method(uint16_t method_id, Method_handler h);
    bool register_notification(uint16_t method_id, Notification_handler h);

    bool request_async(ipc::Request& r, const On_response& h, ipc::Busy_policy policy);
    bool notify_async(ipc::Notification& n);

private:
    std::shared_ptr<ipc::Client> impl_;
};

}
}