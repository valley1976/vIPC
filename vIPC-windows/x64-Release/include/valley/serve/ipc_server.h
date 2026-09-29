#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include "valley/base/lang/any.h"
#include "valley/serve/ipc/frame.h"

#include "event_loop.h"

namespace valley {
namespace serve {

namespace ipc {
class Server;

template<typename Handler>
class Stream;
}

struct LIBVALLEY_SERVE_EXPORT Ipc_session
{
    using session_type = ipc::Stream<ipc::Server>;
    using Ptr = std::shared_ptr<session_type>;

    static uint64_t get_id(const Ptr& ses);
    static base::Any& get_user_data(const Ptr& ses);
    // copy buffer then send async
    static bool send_async(const Ptr& ses, const void* buffer, size_t size);
    static bool disconnect_async(const Ptr& ses);
};

class LIBVALLEY_SERVE_EXPORT Ipc_server
{
public:
    using session_type = ipc::Stream<ipc::Server>;
    using Session_ptr  = std::shared_ptr<session_type>;

    using Method_handler = std::function<void(const Session_ptr&, uint16_t/*method*/, uint32_t/*sequence*/, const void*/*payload*/, uint16_t/*payload_size*/, ipc::Response&/*response*/) >;
    using Notification_handler = std::function<void(const Session_ptr&, uint16_t/*method*/, uint32_t/*sequence*/, const void*/*payload*/, uint16_t/*payload_size*/)>;
    using On_response = std::function<void(uint16_t/*method*/, uint32_t/*sequence*/, uint32_t/*status*/, const void*/*payload*/, uint16_t/*payload_size*/)>;

public:
    Ipc_server(Event_loop& event_loop, const std::string& address);

    Ipc_server(const Ipc_server&) = delete;
    Ipc_server& operator=(const Ipc_server&) = delete;

    const std::string& path() const noexcept;

    uint64_t connected_sessions() const noexcept;

    bool is_keep_alive() const noexcept;
    bool is_no_delay() const noexcept ;
    bool is_reuse_address() const noexcept;
    bool is_reuse_port() const noexcept;

    //! Is the server started?
    bool is_started() const noexcept;

    bool start();
    bool stop();
    bool restart();

    bool broadcast(const void* buffer, size_t size);

    bool disconnect_all();

    Ipc_session::Ptr find_session(uint64_t id);

    void setup_keep_alive(bool enable) noexcept;
    void setup_no_delay(bool enable) noexcept;
    void setup_reuse_address(bool enable) noexcept;
    void setup_reuse_port(bool enable) noexcept;

    bool register_method(uint16_t method_id, Method_handler h);
    bool register_notification(uint16_t method_id, Notification_handler h);

    bool request_async(uint64_t session_id, ipc::Request& request, const On_response& h, ipc::Busy_policy policy = ipc::Busy_policy::kRejectIfBusy);
    bool notify_async(uint64_t session_id, ipc::Notification& n);
    //bool broadcast_notify(const ipc::Notification& n);

private:
    std::shared_ptr<ipc::Server> get_impl() { return impl_; }

private:
    std::shared_ptr<ipc::Server> impl_;
};

}
}