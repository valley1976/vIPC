#pragma once

#include <functional>
#include <memory>
#include <string>

#include "export.h"
#include "message.h"

namespace valley {
namespace serve {
namespace internal {
class Publisher;
}

class LIBVALLEY_SERVE_EXPORT Publisher
{
public:
    Publisher();
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    Publisher(Publisher&& orig) noexcept;
    Publisher& operator=(Publisher&& orig) noexcept;

    static Publisher create(const std::string& url);

    explicit operator bool() const { return impl_ != nullptr; }

    bool send(Message&& msg);

private:
    std::unique_ptr<internal::Publisher> impl_;
};

}
}