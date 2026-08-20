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

    std::shared_ptr<internal::Context>& impl();

    void dispatch(const Task& task);
    void post(const Task& task);

private:
    std::shared_ptr<internal::Context> impl_;
};

}
}