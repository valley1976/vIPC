#pragma once

#include <string>
#include <functional>
#include <memory>

#include "valley/ipc/export.h"
#include "valley/base/lang/any.h"

#include "message.h"
#include "context.h"

namespace valley {
namespace ipc {
namespace internal 
{
    class Connection;
    class Server;

    using On_accept       = std::function<void(size_t, Connection&, base::Any&)>;
    using On_message      = std::function<void(size_t, Connection&, const Message_view&, base::Any&)>;
    using On_disconnected = std::function<void(size_t)>;

    void LIBVALLEY_IPC_EXPORT send_async(Connection& connection, Message& msg);
}

class LIBVALLEY_IPC_EXPORT Server
{
public:
    explicit Server(const std::string& name);
    Server(Context& context, const std::string& name);
    virtual ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void on_accept(internal::On_accept fn);
    void on_message(internal::On_message fn);
    void on_disconnected(internal::On_disconnected on_disconnected);

    void enable_heartbeat();

    void start();
    void stop();

    bool send_async(size_t connection_id, Message& msg);

private:
    std::shared_ptr<internal::Server> impl_;
};

}
}