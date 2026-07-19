#pragma once

#include <chrono>
#include <memory>

#include <valley/lang/storage.h>

namespace valley {
namespace thread {

namespace internal {
class Channel;
}

class Channel
{
public:
    Channel(size_t subscriber_count = 1, size_t data_size = 64, size_t cached_count = 8);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    class Publisher
    {
    public:
        Publisher(Channel& channel);
        ~Publisher();

        Publisher(const Publisher&) = delete;
        Publisher& operator=(const Publisher&) = delete;

        size_t max_data_size() const;

        void* get(size_t size);
        void write();

        template<typename T>
        T* get();
        template<typename T>
        void write(const T& val);

    private:
        class Impl;
        lang::Storage<Impl, 16, 8> impl_;
    };

    struct Subscriber
    {
        Subscriber(Channel& channel, int id);
        ~Subscriber();

        Subscriber(const Subscriber&) = delete;
        Subscriber& operator=(const Subscriber&) = delete;

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
        void catch_up(const Fn& fn);
        template<typename T, typename Fn>
        void catch_up(const Fn& fn);

    private:
        class Impl;
        lang::Storage<Impl, 32, 8> impl_;
    };

    size_t max_data_size() const;
    size_t max_cached_size() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// inline

// Publisher

template<typename T>
inline T* Channel::Publisher::get()
{
    static_assert(std::is_standard_layout<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());

    return reinterpret_cast<T*>(get(sizeof(T)));
}

template<typename T>
inline void Channel::Publisher::write(const T& val)
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());
    *reinterpret_cast<T*>(get(sizeof(T))) = val;

    write();
}

// Subscriber

template<typename T>
inline const T* Channel::Subscriber::read_latest()
{
    assert(sizeof(T) <= max_data_size());
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

    size_t size;
    auto* ptr = reinterpret_cast<T*>(read_latest(size));
    if (ptr) {
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
    assert(sizeof(T) <= max_data_size());
    size_t size;
    auto* ptr = reinterpret_cast<const T*>(read(size));
    assert(ptr?size == sizeof(T) : true);
    return ptr;
}

template<typename T>
inline bool Channel::Subscriber::read(T& value)
{
    static_assert(std::is_trivially_copyable<T>::value, "bad T");
    assert(sizeof(T) <= max_data_size());

    size_t size;
    auto* ptr = reinterpret_cast<T*>(read(size));
    if (ptr) {
        assert(size == sizeof(T));
        value = *ptr;
        return true;
    }
    else {
        return false;
    }
}

template<typename Fn>
inline void Channel::Subscriber::catch_up(const Fn& fn)
{
    size_t size;
    auto* ptr = read(size);
    if (ptr) fn(ptr, size);

    while ((ptr = read(size)))
        fn(ptr, size);
}

template<typename T, typename Fn>
inline void Channel::Subscriber::catch_up(const Fn& fn)
{
    auto* ptr = read<T>();
    if (ptr) fn(ptr);

    while ((ptr = read<T>()))
        fn(ptr);
}

}
}