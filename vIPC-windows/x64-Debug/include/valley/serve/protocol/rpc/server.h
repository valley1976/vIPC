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

class LIBVALLEY_SERVE_EXPORT Rpc_server
{
public:
    using Handler = std::function<Result<Message>(const Message&)>;

    /// Notification handler — fire-and-forget, no return value
    using Notification_handler = std::function<void(const Message&)>;

    Rpc_server();
    ~Rpc_server();

    Rpc_server(const Rpc_server&) = delete;
    Rpc_server& operator=(const Rpc_server&) = delete;

    Rpc_server(Rpc_server&& orig) noexcept;
    Rpc_server& operator=(Rpc_server&& orig) noexcept;

    static Rpc_server create_tcp_server(Context& context, const std::string& host, uint16_t port, const std::function<std::error_code>& cb);
    static Rpc_server create_local_server(Context& context, const std::string& path, const std::function<std::error_code>& cb);

    explicit operator bool() const { return impl_ != nullptr; }

    Result<void> register_method(uint32_t method_id, const Handler& handler);
    Result<void> register_notification_handler(uint32_t method_id, const Notification_handler& handler);

    void start();
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}