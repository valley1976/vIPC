#pragma once

#include <functional>
#include <memory>
#include <system_error>

#include "valley/base/lang/any.h"
#include "valley/base/lang/uuid.h"
#include "valley/serve/core/bytes.h"

namespace valley {
namespace serve {

namespace tcp {
class Session;
}

class Tcp_session;

struct Tcp_event
{
    using On_connected      = std::function<void(Tcp_session&)>;
    using On_received       = std::function<void(Tcp_session&, const Bytes_type&)>;
    using On_disconnected   = std::function<void(Tcp_session&)>;
    using On_error          = std::function<void(Tcp_session&, std::error_code)>;

    On_connected            on_connected;
    On_disconnected         on_disconnected;
    On_received             on_received;
    On_error                on_error;
};

class LIBVALLEY_SERVE_EXPORT Tcp_session
{
public:
    explicit Tcp_session(tcp::Session& impl);
    ~Tcp_session() = default;

    Tcp_session(const Tcp_session&) = delete;
    Tcp_session& operator=(const Tcp_session&) = delete;

    Tcp_session(Tcp_session&& orig) noexcept = delete;
    Tcp_session& operator=(Tcp_session&& orig) noexcept = delete;

    tcp::Session& session() { return impl_; }

    const base::UUID& id() const;

    base::Any& user_data() noexcept;
    const base::Any& user_data() const noexcept;

    bool send_async(Bytes&& bytes);

    bool receive_async(size_t size = 4 * 1024, size_t at_least = 0);
    bool receive_exactly_async(size_t size);
    void consume_received(size_t bytes);

    void disconnect();

private:
    tcp::Session& impl_;
};

}
}