#pragma once

#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {

namespace internal {
class Channel;
}

class LIBVALLEY_IPC_EXPORT Channel
{
public:
    Channel();
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&& orig) noexcept;
    Channel& operator=(Channel&& orig) noexcept;

    static Channel create(const std::string& topic);

    explicit operator bool() const noexcept { return static_cast<bool>(impl_); }

    const std::string& topic() const;
    size_t max_data_size() const;
    size_t max_cached_size() const;

    class Publisher;
    class Subscriber;

private:
    std::shared_ptr<internal::Channel> impl_;
};

class LIBVALLEY_IPC_EXPORT Channel::Publisher
{
public:
    Publisher();
    ~Publisher();

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    Publisher(Publisher&& orig) noexcept;
    Publisher& operator=(Publisher&& orig) noexcept;

    static Publisher create(Channel& channel);

    explicit operator bool() const noexcept { return static_cast<bool>(impl_); }

    size_t max_data_size() const;

    void* get(size_t size);
    void write();

    template<typename T>
    T* get();
    template<typename T>
    void write(const T& val);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class LIBVALLEY_IPC_EXPORT Channel::Subscriber
{
public:
    Subscriber();
    ~Subscriber();

    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;

    Subscriber(Subscriber&& orig) noexcept;
    Subscriber& operator=(Subscriber&& orig) noexcept;

    static Subscriber create(Channel& channel, const std::string& name);

    explicit operator bool() const noexcept { return static_cast<bool>(impl_); }

    const void* read_latest(size_t& size);
    const void* read(size_t& size);

    using Time_point = std::chrono::high_resolution_clock::time_point;

    const void* read_latest(size_t& size, uint64_t& sequence, Time_point& writed_time);
    const void* read(size_t& size, uint64_t& sequence, Time_point& writed_time);

    size_t max_data_size() const;

    template<typename T>
    const T* read_latest();
    template<typename T>
    bool read_latest(T& value);

    template<typename T>
    const T* read();
    template<typename T>
    bool read(T& value);

    template<typename Fn>
    void read_all(const Fn& fn);
    template<typename T, typename Fn>
    void read_all(const Fn& fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// inline

// Publisher

template<typename T>
inline T* Channel::Publisher::get()
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);

    return reinterpret_cast<T*>(get(sizeof(T)));
}

template<typename T>
inline void Channel::Publisher::write(const T& val)
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);

    *reinterpret_cast<T*>(get(sizeof(T))) = val;
    write();
}

// Subscriber

template<typename T>
inline const T* Channel::Subscriber::read_latest()
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);
    
    size_t size;
    auto* ptr = reinterpret_cast<const T*>(read_latest(size));
    assert(ptr ? size == sizeof(T) : true);

    return ptr;
}

template<typename T>
inline bool Channel::Subscriber::read_latest(T& value)
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);

    size_t size;
    auto ptr = reinterpret_cast<const T*>(read_latest(size));
    if (ptr)
    {
        assert(size == sizeof(T));
        value = *ptr;
        return true;
    }
    else {
        return false;
    }
}

template<typename T>
inline const T* Channel::Subscriber::read()
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);

    size_t size;
    auto* ptr = reinterpret_cast<const T*>(read(size));
    assert(ptr? size == sizeof(T): true);

    return ptr;
}

template<typename T>
inline bool Channel::Subscriber::read(T& value)
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    assert(impl_);

    size_t size;
    auto ptr = reinterpret_cast<const T*>(read(size));
    if (ptr)
    {
        assert(size == sizeof(T));
        value = *ptr;
        return true;
    }
    else {
        return false;
    }
}

template<typename Fn>
inline void Channel::Subscriber::read_all(const Fn& fn)
{
    size_t size;
    const void* ptr;
    while((ptr = read(size)) != nullptr)
        fn(ptr, size);
}

template<typename T, typename Fn>
inline void Channel::Subscriber::read_all(const Fn& fn)
{
    const T* ptr;
    while ((ptr = read<T>()) != nullptr)
        fn(ptr);
}

}
}