#pragma once

#include <functional>
#include <memory>
#include <string>

#include "export.h"
#include "message.h"

namespace valley {
namespace serve {
namespace internal {
class Subscriber;
}

class LIBVALLEY_SERVE_EXPORT Subscriber
{
public:
    Subscriber();
    ~Subscriber();

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;

    Subscriber(Subscriber&& orig) noexcept;
    Subscriber& operator=(Subscriber&& orig) noexcept;


    using On_message = std::function<void(Message&)>;
    static Subscriber create(const std::string& url, const On_message& fn, const std::string& filter = "");

    explicit operator bool() const { return impl_ != nullptr; }

private:
    std::unique_ptr<internal::Subscriber> impl_;
};

}
}