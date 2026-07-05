#pragma once

#include <functional>
#include <memory>

namespace valley {
namespace local {
namespace internal {
class Context;
}

class Server;
class Client;

class Context
{
public:
    using On_start = std::function<void()>;
    using On_exception = std::function<void(const std::exception&)>;

    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void start(const On_exception& on_exception = nullptr, const On_start& on_start = nullptr);
    void stop();

private:
    friend Server;
    friend Client;

private:
    std::shared_ptr<internal::Context> impl_;
};


}
}