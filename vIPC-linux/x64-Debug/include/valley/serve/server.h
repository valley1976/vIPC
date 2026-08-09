#pragma once

#include <functional>
#include <memory>
#include <string>

#include "export.h"
#include "message.h"

namespace valley {
namespace serve {
namespace internal {
    class Server;
}

class LIBVALLEY_SERVE_EXPORT Server
{
public:
    using Session = void*;
    using On_received_message = std::function<void(Session, Message&)>;

    Server();
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&& orig) noexcept;
    Server& operator=(Server&& orig) noexcept;

    static Server create(const std::string& url, size_t max_client, const On_received_message& fn);

    explicit operator bool() const { return impl_ != nullptr; }

    void send(Session session, Message&& msg);

private:
    std::unique_ptr<internal::Server> impl_;
};

}
}