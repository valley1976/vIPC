#pragma once

#include <memory>

#include "valley/serve/serve.h"

namespace valley {
namespace serve {

namespace internal {
    class BidiRpcNode;
}

class Rpc_node;

class Rpc_message
{
public:
    Rpc_message(Rpc_message&& orig) noexcept;
    Rpc_message& operator=(Rpc_message&& orig) noexcept;

    static Rpc_message make_request(uint16_t method_id, size_t size);
    static Rpc_message make_response(uint16_t method_id, uint32_t cid, size_t size = 0, Status status = Status::kOk);
    static Rpc_message make_notification(uint16_t method_id, size_t size);

    bool is_request() const;
    bool is_response() const;
    bool is_nitification() const;

    Byte* data()                { return bytes_.data(); }
    const Byte* data() const    { return bytes_.data(); }

    size_t size() const         { return bytes_.size(); }

    void resize_payload(size_t size);

    Byte* payload();
    const Byte* payload() const;
    
    size_t payload_size() const;

protected:
    friend Rpc_node;

    Rpc_message(size_t size);

    void set_call_id(uint32_t id);

private:
    Bytes bytes_;
};

class Rpc_request : public Rpc_message
{
public:
    Rpc_request(uint16_t method_id, size_t size);
};

class Rpc_response : public Rpc_message
{
public:
    Rpc_response(uint16_t method_id, uint32_t cid, size_t size = 0, Status status = Status::kOk);
};

class Rpc_notification : public Rpc_message
{
public:
    Rpc_notification(uint16_t method_id, size_t size);
};

class Rpc_node
{
public:
    Rpc_node();
    ~Rpc_node();

    Rpc_node(const Rpc_node&) =delete;
    Rpc_node& operator=(const Rpc_node&) = delete;

    Rpc_node(Rpc_node&& orig) noexcept;
    Rpc_node& operator=(Rpc_node&& orig) noexcept;

    void start_listen(const std::string& host, uint16_t port, const std::function<void(std::error_code)>& cb = nullptr);
    uint32_t connect_to(const std::string& host, uint16_t port, const std::function<void(std::error_code)>& cb = nullptr);

    using Handler = std::function<Result<Bytes>(const Byte*, size_t)>;
    void register_method(uint32_t method_id, Handler handler); 

    using Callback = std::function<void(Error, const Byte*, size_t)>;
    bool call_async(uint32_t peer_id, Rpc_request& req, Callback callback);

    bool notify(uint32_t peer_id, Rpc_notification& nty);

private:
    std::unique_ptr<internal::BidiRpcNode> impl_;
};

}
}