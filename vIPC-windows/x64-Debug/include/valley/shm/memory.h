#pragma once

#include <memory>
#include <string>

namespace valley {
namespace shm {

class Memory
{
public:
    Memory();
    ~Memory();

    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    Memory(Memory&& orig) noexcept;
    Memory& operator=(Memory&& orig) noexcept;

    static Memory create(const std::string& name, size_t size, bool global = false);
    static Memory open(const std::string& name, bool global = false);

    explicit operator bool() const { return impl_ != nullptr; }

    const std::string& name() const noexcept;
    size_t size() const noexcept;

    void* ptr();
    const void* ptr() const;

    bool owner() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}