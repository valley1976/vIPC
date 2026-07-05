#pragma once

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace valley {
namespace lang {

template<typename T>
class Inplace
{
public:
    Inplace(void* ptr, size_t size) : object_(static_cast<T*>(ptr)),
        constructed_(false)
    {
        if (reinterpret_cast<uintptr_t>(ptr) % alignof(T) != 0)
            throw std::invalid_argument("inplace: bad alignment");

        if (size < sizeof(T))
            throw std::invalid_argument("inplace: buffer too small");
    }

    ~Inplace()
    {
        destroy();
    }

    Inplace(const Inplace&) = delete;
    Inplace& operator=(const Inplace&) = delete;

    Inplace(Inplace&& other) noexcept
        : object_(other.object_), constructed_(other.constructed_)
    {
        other.object_ = nullptr;
        other.constructed_ = false;
    }

    Inplace& operator=(Inplace&& other) noexcept {
        if (this != &other) {
            destroy();

            object_ = other.object_;
            constructed_ = other.constructed_;
            other.object_ = nullptr;
            other.constructed_ = false;
        }
        return *this;
    }

    template<typename ...Args>
    void construct(Args&& ...args)
    {
        if (constructed_)
            throw std::logic_error("inplace: already constructed");
        new (object_) T(std::forward<Args>(args)...);
        constructed_ = true;
    }

    T* operator->() { assert(constructed_); return object_; }
    const T* operator->() const { assert(constructed_); return object_; }

    T& operator*() { assert(constructed_); return *object_; }
    const T& operator*() const { assert(constructed_); return *object_; }

private:
    void destroy()
    {
        if (constructed_) {
            object_->~T();
            constructed_ = false;
        }
    }

private:
    T* object_;
    bool constructed_;
};

}
}