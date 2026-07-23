#pragma once

#include <memory>
#include <string>

#include "valley/export/export.h"

namespace valley {
namespace exec {

class LIBVALLEY_EXPORT Notification
{
public:
    Notification();
    Notification(const std::string& executor, const std::string& task);
    ~Notification();

    Notification(const Notification&) = delete;
    Notification operator =(const Notification&) = delete;

    Notification(Notification&& orig) noexcept;
    Notification& operator=(Notification&& orig) noexcept;

    bool is_valid() const { return impl_ != nullptr; }

    void emit();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}