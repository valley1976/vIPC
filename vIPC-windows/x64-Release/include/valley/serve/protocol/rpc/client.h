#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <system_error>

#include "valley/serve/export.h"
#include "valley/serve/core/context.h"
#include "valley/serve/core/message.h"
#include "valley/serve/core/result.h"

namespace valley {
namespace serve {

class LIBVALLEY_SERVE_EXPORT Rpc_client
{
public:
    using Handler = std::function<Result<Message>(const Message&)>;
    using Notification_handler = std::function<void(const Message&)>;

    Rpc_client();
    ~Rpc_client();

    Rpc_client(const Rpc_client&) = delete;
    Rpc_client& operator=(const Rpc_client&) = delete;

    Rpc_client(Rpc_client&& orig) noexcept;
    Rpc_client& operator=(Rpc_client&& orig) noexcept;

    static Rpc_client create_tcp_client(Context& context, const std::string& host, uint16_t port, const std::function<std::error_code>& cb);
    static Rpc_client create_local_client(Context& context, const std::string& path, const std::function<std::error_code>& cb);

    explicit operator bool() const { return impl_ != nullptr; }

    uint32_t async_call(uint32_t method_id, const Message& request,
        uint32_t timeout_ms,
        std::function<void(Error, Message)> callback);

    void send_notification(uint32_t method_id, const Message& payload);

    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}