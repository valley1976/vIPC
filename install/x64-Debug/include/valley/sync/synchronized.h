#pragma once

#include <mutex>
#include <type_traits>
#include <utility>

#include <valley/lang/cleanup.h>

namespace valley {
namespace sync {

template<typename T, typename Mutex = std::mutex>
class Synchronized
{
public:
    template<typename ...Args>
    Synchronized(Args&& ... args) : mutex_(),
        object_(std::forward<Args>(args)...)
    {}

    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;

    template<typename Fn>
    auto apply(const Fn& fn) -> decltype(fn(std::declval<bool>(), std::declval<T&>()))
    {
        std::lock_guard<Mutex> guard{ mutex_ };

        return fn(object_);
    }

    template<typename Fn>
    auto apply(const Fn& fn) const -> decltype(fn(std::declval<bool>(), std::declval<T&>()))
    {
        std::lock_guard<Mutex> guard{ mutex_ };

        return fn(object_);
    }

    template<typename Fn>
    bool try_apply(Fn&& fn)
    {
        if (mutex_.try_lock())
        {
            auto scope_exit = lang::make_cleanup([this]() { mutex_.unlock(); });

            std::forward<Fn>(fn)(object_);

            return true;
        }

        return false;
    }

    template<typename Fn>
    bool try_apply(Fn&& fn) const
    {
        if (mutex_.try_lock())
        {
            auto scope_exit = lang::make_cleanup([this]() { mutex_.unlock(); });

            std::forward<Fn>(fn)(object_);

            return true;
        }

        return false;
    }

    class Ptr
    {
    public:
        explicit Ptr(Synchronized& sync) : lock_(sync.mutex_),
            ptr_(&sync.object_)
        {
        }

        Ptr(const Ptr&) = delete;
        Ptr& operator=(const Ptr&) = delete;

        Ptr(Ptr&& orig) noexcept
            : lock_(std::move(orig.lock_)),
            ptr_(orig.ptr_)
        {
            orig.ptr_ = nullptr;
        }

        Ptr& operator=(Ptr&& orig) noexcept
        {
            if (this != &orig)
            {
                lock_ = std::move(orig.lock_);
                ptr_ = orig.ptr_;

                orig.ptr_ = nullptr;
            }

            return *this;
        }

        operator bool() const { return nullptr != ptr_; }

        T& operator* () const noexcept { return *ptr_; }
        T* operator->() const noexcept { return ptr_; }

    private:
        std::unique_lock<Mutex> lock_;
        T* ptr_;
    };

    class Const_ptr
    {
    public:
        Const_ptr(const Synchronized& sync) : lock_(sync.mutex_),
            ptr_(&sync.object_)
        {
        }

        Const_ptr(const Const_ptr&) = delete;
        Const_ptr& operator=(const Const_ptr&) = delete;

        Const_ptr(Const_ptr&& orig) noexcept
            : lock_(std::move(orig.lock_)),
            ptr_(orig.ptr_)
        {
            orig.ptr_ = nullptr;
        }

        Const_ptr& operator=(Const_ptr&& orig) noexcept
        {
            if (this != &orig)
            {
                lock_ = std::move(orig.lock_);
                ptr_ = orig.ptr_;

                orig.ptr_ = nullptr;
            }

            return *this;
        }

        operator bool() const { return nullptr != ptr_; }

        const T& operator* () const noexcept { return *ptr_; }
        const T* operator->() const noexcept { return ptr_; }

    private:
        std::unique_lock<Mutex> lock_;
        const T* const ptr_;
    };

    Ptr get() { return Ptr(*this); }
    Const_ptr get() const { return Const_ptr(*this); }

private:
    mutable Mutex mutex_;
    T object_;
};

}
}