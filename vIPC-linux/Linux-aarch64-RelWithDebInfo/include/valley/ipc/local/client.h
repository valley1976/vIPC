#pragma once

#include <string>
#include <chrono>
#include <memory>

#include "valley/ipc/export.h"

#include "message.h"
#include "context.h"

namespace valley {
namespace ipc {
namespace internal {
    class Client;
    using On_available = std::function<void(Client&)>;
    using Message_handler = std::function<void(internal::Client&, const Message_view&)>;

    void LIBVALLEY_IPC_EXPORT send_async(Client& connection, Message& msg);
}

using Seconds       = std::chrono::seconds;
using Milliseconds  = std::chrono::milliseconds;

class LIBVALLEY_IPC_EXPORT Client
{
public:
    explicit Client(const std::string& name);
    Client(Context& context, const std::string& name);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void on_message(internal::Message_handler fn);
    void on_available(internal::On_available fn);

    void start();

    void request(Message& msg, const internal::Message_handler& fn, const Milliseconds& timeout = Seconds(3));
    void send_async(Message& msg);

    void stop();

private:
    std::shared_ptr<internal::Client> impl_;
};

}
}