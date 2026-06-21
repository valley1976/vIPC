#pragma once

#include <memory>
#include <string>

namespace valley {
namespace exec {

class Notification
{
public:
    Notification(const std::string& executor, const std::string& task);
    ~Notification();

    Notification(const Notification&) = delete;
    Notification operator =(const Notification&) = delete;

    void emit();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}