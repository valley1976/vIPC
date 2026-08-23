#pragma once

#include <memory>
#include <functional>

#include "valley/serve/export.h"

namespace valley {
namespace serve {
namespace internal {
    class Context;
}

using Task = std::function<void()>;

class LIBVALLEY_SERVE_EXPORT Context
{
public:
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& orig);
    Context& operator=(Context&& orig);

    const std::shared_ptr<internal::Context>& impl() const;

    void dispatch(const Task& task);
    void post(const Task& task);

    void start();
    void stop();

private:
    std::shared_ptr<internal::Context> impl_;
};

}
}