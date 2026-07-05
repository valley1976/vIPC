#pragma once

#include <string>
#include <functional>
#include <memory>

#include "valley/lang/any.h"

#include "message.h"
#include "context.h"

namespace valley {
namespace local {
namespace internal 
{
    class Connection;
    class Server;

    using On_accept       = std::function<void(size_t, Connection&, lang::Any&)>;
    using On_message      = std::function<void(size_t, Connection&, const Message_view&, lang::Any&)>;
    using On_disconnected = std::function<void(size_t)>;

    void send_async(Connection& connection, Message& msg);
}

class Server
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

    void start();
    void stop();

    bool send_async(size_t connection_id, Message& msg);

private:
    std::shared_ptr<internal::Server> impl_;
};

}
}