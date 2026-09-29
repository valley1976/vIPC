#pragma once

#include <cstddef>
#include <chrono>
#include <memory>
#include <functional>
#include <string>

#include "frame.h"
#include "valley/serve/local_server.h"

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {
namespace serve {

namespace internal {
class Server;
}

using valley::serve::Event_loop;

class LIBVALLEY_IPC_EXPORT Server
{
public:
    using u32 = Protocol::u32;
    using Byte = Protocol::Byte;
    using Session_ptr = valley::serve::Local_session::Ptr; 

    using On_response          = std::function<void(const void*, size_t)>;
    using Method_handler       = std::function<void(const Session_ptr&, const void*, size_t, Frame& /* in&out response*/) >;
    using Notification_handler = std::function<void(const Session_ptr&, const void*, size_t)>;

    Server(Event_loop& ev, const std::string& name);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    bool register_method(Protocol::u16 method_id, Method_handler h);
    bool register_notification(Protocol::u16 method_id, Notification_handler h);

    enum class Busy_policy {
        kRejectIfBusy,
        kCancelAndReplace
    };

    bool request_async(uint64_t id, const Frame& request, const On_response& h, Busy_policy policy = Busy_policy::kRejectIfBusy);
    bool broadcast_notify(const Frame& event);

    bool start();
    bool stop();
    bool restart();

private:
    std::unique_ptr<internal::Server> impl_;
};

}
}
}