#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <string>

#include "frame.h"
#include "valley/serve/event_loop.h"

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {
namespace serve {

using valley::serve::Event_loop;

class LIBVALLEY_IPC_EXPORT Server
{
public:
    using u32 = Protocol::u32;
    using Byte = Protocol::Byte;
    using Frame = Frame;

    using Method_handler       = std::function<void(u32/*status*/, const Byte*, size_t, Frame& /* in&out response*/) >;
    using Notification_handler = std::function<void(const Byte*, size_t)>;

    Server(Event_loop& ev, const std::string& name);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    bool register_method(Protocol::u16 method_id, Method_handler h);
    bool register_notification(Protocol::u16 method_id, Notification_handler h);

    bool broadcast_notify(Protocol::u16 method_id, const void* data, size_t size);

    bool start();
    bool stop();
    bool restart();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}
}