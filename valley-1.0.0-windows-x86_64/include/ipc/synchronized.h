// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "ipc/robust_mutex.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
#  include <intrin.h>
#  include <process.h>
#else
#  include <unistd.h>
#endif

namespace ipc {

// 与 shm::Region_view 布局兼容的轻量视图。
// 调用方也可以直接传任意具有 data/size 成员的 view。
struct Region_view
{
    void* data = nullptr;
    std::size_t size = 0;
    std::string name;
};

// 在 shm::Region_view 上放置 Control + Robust_mutex + T。
// Synchronized 本身是进程本地句柄；共享内存中的 Robust_mutex/T 只构造一次。
template<typename T>
class Synchronized
{
    static_assert(std::is_trivially_destructible<T>::value,
                  "Synchronized<T> requires trivially destructible T");
    static_assert(std::is_trivially_copyable<T>::value,
                  "Synchronized<T> requires trivially copyable T");
    static_assert(std::is_standard_layout<T>::value,
                  "Synchronized<T> requires standard-layout T");

public:
    struct Options
    {
        std::string mutex_name;
        std::function<void(T&)> repair;
    };

    class Ptr;
    class Const_ptr;

    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;

    Synchronized(Synchronized&& orig) noexcept
        : control_{orig.control_}
        , mutex_{orig.mutex_}
        , object_{orig.object_}
        , options_{std::move(orig.options_)}
    {
        orig.control_ = nullptr;
        orig.mutex_ = nullptr;
        orig.object_ = nullptr;
    }

    Synchronized& operator=(Synchronized&& orig) noexcept
    {
        if (this != &orig) {
            control_ = orig.control_;
            mutex_ = orig.mutex_;
            object_ = orig.object_;
            options_ = std::move(orig.options_);
            orig.control_ = nullptr;
            orig.mutex_ = nullptr;
            orig.object_ = nullptr;
        }
        return *this;
    }

    template<typename View, typename... Args>
    [[nodiscard]] static Synchronized create(const View& view,
                                             Options options,
                                             Args&&... args)
    {
        validate_region(view, true);

        auto* control = static_cast<Control*>(view.data);

        if (load_u32(&control->state) != STATE_EMPTY) {
            throw std::runtime_error{
                "Synchronized::create: region is already initialized"};
        }

        std::uint32_t expected = STATE_EMPTY;
        if (!compare_exchange_u32(&control->state, expected,
                                  STATE_INITIALIZING)) {
            throw std::runtime_error{
                "Synchronized::create: region is being initialized"};
        }

        // Control 是 POD；CAS 后逐字段初始化，保留 state 的 INITIALIZING。
        control->magic = 0;
        control->version = 0;
        control->corrupt = 0;
        control->reserved = 0;
        control->object_size = 0;
        control->object_alignment = 0;
        control->mutex_size = 0;
        control->mutex_alignment = 0;
        control->owner_pid = 0;

        Robust_mutex* mutex = nullptr;
        try {
            mutex = new (static_cast<std::byte*>(view.data) + MUTEX_OFFSET)
                Robust_mutex(options.mutex_name);
            auto* object = new (static_cast<std::byte*>(view.data) + OBJECT_OFFSET)
                T(std::forward<Args>(args)...);

            control->version = VERSION;
            control->object_size = sizeof(T);
            control->object_alignment = alignof(T);
            control->mutex_size = sizeof(Robust_mutex);
            control->mutex_alignment = alignof(Robust_mutex);
            control->owner_pid = process_id();
            control->magic = MAGIC;
            store_u32(&control->state, STATE_READY);
            return Synchronized{control, mutex, object, std::move(options)};
        } catch (...) {
            store_u32(&control->corrupt, 1);
            store_u32(&control->state, STATE_CORRUPT);
            throw;
        }
    }

    // pool slot 入口：base 指向 slot 起始地址，size 是该 slot 的容量。
    template<typename... Args>
    [[nodiscard]] static Synchronized create_at(void* base, std::size_t size,
                                                Options options,
                                                Args&&... args)
    {
        return create(Region_view{base, size}, std::move(options),
                      std::forward<Args>(args)...);
    }

    [[nodiscard]] static Synchronized open_at(void* base, std::size_t size,
                                              Options options = {})
    {
        return open(Region_view{base, size}, std::move(options));
    }

    template<typename View>
    [[nodiscard]] static Synchronized open(const View& view,
                                           Options options = {})
    {
        validate_region(view, false);

        auto* control = static_cast<Control*>(view.data);
        if (load_u32(&control->state) != STATE_READY) {
            throw std::runtime_error{
                "Synchronized::open: region is not ready"};
        }
        if (control->magic != MAGIC ||
            control->version != VERSION ||
            control->object_size != sizeof(T) ||
            control->object_alignment != alignof(T) ||
            control->mutex_size != sizeof(Robust_mutex) ||
            control->mutex_alignment != alignof(Robust_mutex) ||
            load_u32(&control->corrupt) != 0) {
            throw std::runtime_error{
                "Synchronized::open: region is not a valid ready object"};
        }

        auto* mutex = reinterpret_cast<Robust_mutex*>(
            static_cast<std::byte*>(view.data) + MUTEX_OFFSET);
        auto* object = reinterpret_cast<T*>(
            static_cast<std::byte*>(view.data) + OBJECT_OFFSET);
        return Synchronized{control, mutex, object, std::move(options)};
    }

    [[nodiscard]] static constexpr std::size_t required_size()
    {
        return OBJECT_OFFSET + sizeof(T);
    }

    [[nodiscard]] static constexpr std::size_t required_alignment()
    {
        return alignof(Control) > alignof(Robust_mutex)
                   ? (alignof(Control) > alignof(T) ? alignof(Control)
                                                    : alignof(T))
                   : (alignof(Robust_mutex) > alignof(T) ? alignof(Robust_mutex)
                                                  : alignof(T));
    }

    template<typename Fn>
    auto apply(Fn&& fn)
        -> decltype(std::forward<Fn>(fn)(std::declval<T&>()))
    {
        const auto state = mutex_->lock();
        Unlock_guard guard{*mutex_};
        recover_or_throw(state);
        return std::forward<Fn>(fn)(*object_);
    }

    template<typename Fn>
    auto apply(Fn&& fn) const
        -> decltype(std::forward<Fn>(fn)(std::declval<const T&>()))
    {
        const auto state = mutex_->lock();
        Unlock_guard guard{*mutex_};
        recover_or_throw(state);
        return std::forward<Fn>(fn)(*object_);
    }

    template<typename Fn>
    bool try_apply(Fn&& fn)
    {
        Robust_mutex::Lock_state state{};
        if (!mutex_->try_lock(state))
            return false;
        Unlock_guard guard{*mutex_};
        if (!recover_without_throw(state))
            return false;
        std::forward<Fn>(fn)(*object_);
        return true;
    }

    template<typename Fn>
    bool try_apply(Fn&& fn) const
    {
        Robust_mutex::Lock_state state{};
        if (!mutex_->try_lock(state))
            return false;
        Unlock_guard guard{*mutex_};
        if (!recover_without_throw(state))
            return false;
        std::forward<Fn>(fn)(*object_);
        return true;
    }

    template<typename Fn, typename Result>
    bool try_apply(Fn&& fn, Result& result)
    {
        Robust_mutex::Lock_state state{};
        if (!mutex_->try_lock(state))
            return false;
        Unlock_guard guard{*mutex_};
        if (!recover_without_throw(state))
            return false;
        result = std::forward<Fn>(fn)(*object_);
        return true;
    }

    template<typename Fn, typename Result>
    bool try_apply(Fn&& fn, Result& result) const
    {
        Robust_mutex::Lock_state state{};
        if (!mutex_->try_lock(state))
            return false;
        Unlock_guard guard{*mutex_};
        if (!recover_without_throw(state))
            return false;
        result = std::forward<Fn>(fn)(*object_);
        return true;
    }

    [[nodiscard]] Ptr get() { return Ptr{*this}; }
    [[nodiscard]] Const_ptr get() const
    {
        return Const_ptr{const_cast<Synchronized&>(*this)};
    }

private:
    enum : std::uint32_t
    {
        STATE_EMPTY = 0,
        STATE_INITIALIZING = 1,
        STATE_READY = 2,
        STATE_CORRUPT = 3
    };

    static constexpr std::uint64_t MAGIC = 0x53594E4353484D31ULL;
    static constexpr std::uint32_t VERSION = 1;

    struct alignas(64) Control
    {
        std::uint64_t magic = 0;
        std::uint32_t version = 0;
        std::uint32_t state = STATE_EMPTY;
        std::uint32_t corrupt = 0;
        std::uint32_t reserved = 0;
        std::uint64_t object_size = 0;
        std::uint64_t object_alignment = 0;
        std::uint64_t mutex_size = 0;
        std::uint64_t mutex_alignment = 0;
        std::uint64_t owner_pid = 0;
    };

    static std::uint32_t load_u32(const std::uint32_t* address) noexcept
    {
#if defined(_WIN32)
        return static_cast<std::uint32_t>(
            ::_InterlockedCompareExchange(
                reinterpret_cast<volatile long*>(
                    const_cast<std::uint32_t*>(address)),
                0, 0));
#else
        return __atomic_load_n(address, __ATOMIC_ACQUIRE);
#endif
    }

    static void store_u32(std::uint32_t* address,
                          std::uint32_t value) noexcept
    {
#if defined(_WIN32)
        (void)::_InterlockedExchange(
            reinterpret_cast<volatile long*>(address),
            static_cast<long>(value));
#else
        __atomic_store_n(address, value, __ATOMIC_RELEASE);
#endif
    }

    static bool compare_exchange_u32(std::uint32_t* address,
                                     std::uint32_t& expected,
                                     std::uint32_t desired) noexcept
    {
#if defined(_WIN32)
        const long previous = ::_InterlockedCompareExchange(
            reinterpret_cast<volatile long*>(address),
            static_cast<long>(desired), static_cast<long>(expected));
        if (static_cast<std::uint32_t>(previous) != expected) {
            expected = static_cast<std::uint32_t>(previous);
            return false;
        }
        return true;
#else
        return __atomic_compare_exchange_n(
            address, &expected, desired, false, __ATOMIC_ACQ_REL,
            __ATOMIC_ACQUIRE);
#endif
    }

    static constexpr std::size_t align_up(std::size_t value,
                                          std::size_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    static constexpr std::size_t MUTEX_OFFSET =
        align_up(sizeof(Control), alignof(Robust_mutex));
    static constexpr std::size_t OBJECT_OFFSET =
        align_up(MUTEX_OFFSET + sizeof(Robust_mutex), alignof(T));

    class Unlock_guard
    {
    public:
        explicit Unlock_guard(Robust_mutex& mutex) : mutex_{mutex} {}
        ~Unlock_guard() { mutex_.unlock(); }

        Unlock_guard(const Unlock_guard&) = delete;
        Unlock_guard& operator=(const Unlock_guard&) = delete;

    private:
        Robust_mutex& mutex_;
    };

    Synchronized(Control* control, Robust_mutex* mutex, T* object, Options options)
        : control_{control}
        , mutex_{mutex}
        , object_{object}
        , options_{std::move(options)}
    {
    }

    static std::uint64_t process_id() noexcept
    {
#if defined(_WIN32)
        return static_cast<std::uint64_t>(::_getpid());
#else
        return static_cast<std::uint64_t>(::getpid());
#endif
    }

    template<typename View>
    static void validate_region(const View& view, bool creating)
    {
        if (view.data == nullptr || view.size < required_size())
            throw std::invalid_argument{"Synchronized: region is too small"};
        if (reinterpret_cast<std::uintptr_t>(view.data) %
                required_alignment() !=
            0)
            throw std::invalid_argument{"Synchronized: region is misaligned"};

        if (creating && view.size < sizeof(Control))
            throw std::invalid_argument{"Synchronized: region is too small"};
    }

    void recover_or_throw(Robust_mutex::Lock_state state) const
    {
        if (state == Robust_mutex::Lock_state::RECOVERED) {
            if (!options_.repair) {
                store_u32(&control_->corrupt, 1);
                throw std::runtime_error{
                    "Synchronized: owner died without repair callback"};
            }
            try {
                options_.repair(*object_);
            } catch (...) {
                store_u32(&control_->corrupt, 1);
                throw;
            }
        }
        if (load_u32(&control_->corrupt) != 0)
            throw std::runtime_error{"Synchronized: shared object is corrupt"};
    }

    bool recover_without_throw(Robust_mutex::Lock_state state) const noexcept
    {
        if (state == Robust_mutex::Lock_state::RECOVERED) {
            if (!options_.repair) {
                store_u32(&control_->corrupt, 1);
                return false;
            }
            try {
                options_.repair(*object_);
            } catch (...) {
                store_u32(&control_->corrupt, 1);
                return false;
            }
        }
        return load_u32(&control_->corrupt) == 0;
    }

    Control* control_ = nullptr;
    Robust_mutex* mutex_ = nullptr;
    T* object_ = nullptr;
    Options options_;
};

template<typename T>
class Synchronized<T>::Ptr
{
public:
    explicit Ptr(Synchronized& sync) : sync_{&sync}
    {
        Robust_mutex::Lock_state state{};
        try {
            state = sync_->mutex_->lock();
        } catch (...) {
            return;
        }
        locked_ = true;
        if (!sync_->recover_without_throw(state)) {
            sync_->mutex_->unlock();
            locked_ = false;
            return;
        }
        object_ = sync_->object_;
    }

    ~Ptr()
    {
        if (locked_)
            sync_->mutex_->unlock();
    }

    Ptr(const Ptr&) = delete;
    Ptr& operator=(const Ptr&) = delete;

    Ptr(Ptr&& orig) noexcept
        : sync_{orig.sync_}, object_{orig.object_}, locked_{orig.locked_}
    {
        orig.object_ = nullptr;
        orig.locked_ = false;
    }

    Ptr& operator=(Ptr&& orig) noexcept
    {
        if (this != &orig) {
            release();
            sync_ = orig.sync_;
            object_ = orig.object_;
            locked_ = orig.locked_;
            orig.object_ = nullptr;
            orig.locked_ = false;
        }
        return *this;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return object_ != nullptr;
    }

    [[nodiscard]] T& operator*() const noexcept
    {
        assert(object_ != nullptr);
        return *object_;
    }

    [[nodiscard]] T* operator->() const noexcept
    {
        assert(object_ != nullptr);
        return object_;
    }

private:
    void release() noexcept
    {
        if (locked_) {
            sync_->mutex_->unlock();
            locked_ = false;
        }
        object_ = nullptr;
    }

    Synchronized* sync_ = nullptr;
    T* object_ = nullptr;
    bool locked_ = false;
};

template<typename T>
class Synchronized<T>::Const_ptr
{
public:
    explicit Const_ptr(Synchronized& sync) : sync_{&sync}
    {
        Robust_mutex::Lock_state state{};
        try {
            state = sync_->mutex_->lock();
        } catch (...) {
            return;
        }
        locked_ = true;
        if (!sync_->recover_without_throw(state)) {
            sync_->mutex_->unlock();
            locked_ = false;
            return;
        }
        object_ = sync_->object_;
    }

    ~Const_ptr()
    {
        if (locked_)
            sync_->mutex_->unlock();
    }

    Const_ptr(const Const_ptr&) = delete;
    Const_ptr& operator=(const Const_ptr&) = delete;

    Const_ptr(Const_ptr&& orig) noexcept
        : sync_{orig.sync_}, object_{orig.object_}, locked_{orig.locked_}
    {
        orig.object_ = nullptr;
        orig.locked_ = false;
    }

    Const_ptr& operator=(Const_ptr&& orig) noexcept
    {
        if (this != &orig) {
            release();
            sync_ = orig.sync_;
            object_ = orig.object_;
            locked_ = orig.locked_;
            orig.object_ = nullptr;
            orig.locked_ = false;
        }
        return *this;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return object_ != nullptr;
    }

    [[nodiscard]] const T& operator*() const noexcept
    {
        assert(object_ != nullptr);
        return *object_;
    }

    [[nodiscard]] const T* operator->() const noexcept
    {
        assert(object_ != nullptr);
        return object_;
    }

private:
    void release() noexcept
    {
        if (locked_) {
            sync_->mutex_->unlock();
            locked_ = false;
        }
        object_ = nullptr;
    }

    Synchronized* sync_ = nullptr;
    const T* object_ = nullptr;
    bool locked_ = false;
};

}  // namespace ipc
