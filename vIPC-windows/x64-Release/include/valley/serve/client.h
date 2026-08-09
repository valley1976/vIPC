#pragma once

#include <functional>
#include <memory>
#include <string>

#include "message.h"

namespace valley {
namespace serve {
namespace internal {
class Client;
}

class LIBVALLEY_SERVE_EXPORT Client
{
public:
    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& orig) noexcept;
    Client& operator=(Client&& orig) noexcept;

    static Client create(const std::string& url);

    explicit operator bool() const { return impl_ != nullptr; }

    bool request(Message& req, Message& rep);

    using Response_handler = std::function<void(Message&)>;
    bool request_async(Message&& req, const Response_handler& h);

private:
    std::unique_ptr<internal::Client> impl_;
};

}
}