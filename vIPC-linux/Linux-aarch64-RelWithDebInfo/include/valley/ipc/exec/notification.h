#pragma once

#include <memory>
#include <string>

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {

class LIBVALLEY_IPC_EXPORT Notification
{
public:
    Notification();
    ~Notification();

    Notification(const Notification&) = delete;
    Notification operator =(const Notification&) = delete;

    Notification(Notification&& orig) noexcept;
    Notification& operator=(Notification&& orig) noexcept;

    static Notification create(const std::string& executor, const std::string& task);

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    void emit();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}