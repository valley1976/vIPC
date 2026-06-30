#pragma once

#include <cassert>
#include <new>
#include <utility>

namespace valley {
namespace lang {

template<typename T, size_t kSize, size_t kAlign = 4>
struct Storage
{
    alignas(kAlign) char bytes[kSize];

    template<typename ...Args>
    Storage(Args&& ... args)
    {
        assert(kSize >= sizeof(T) && "StorageSize must be increased!");
        assert(alignof(T) % kAlign == 0 && "StorageAlign must be adjusted!");

        new (get())T(std::forward<Args>(args)...);
    }

    ~Storage()
    {
        get()->~T();
    }

    Storage(const Storage& other) { new(get())T(*other); }
    Storage(Storage&& other) { new(get())T(std::move(*other)); }

    Storage& operator=(const Storage& other)
    {
        if (this != &other)
            *get() = *other;

        return *this;
    }

    Storage& operator=(Storage&& other)
    {
        if (this != &other)
            *get() = std::move(*other);

        return *this;
    }

    static constexpr size_t size() { return kSize; }
    static constexpr size_t align() { return kAlign; }

    T* get() { return reinterpret_cast<T*>(bytes); }
    const T* get() const { return reinterpret_cast<const T*>(bytes); }

    T& operator*() { return *get(); }
    const T& operator*() const { return *get(); }

    T* operator->() { return get(); }
    const T* operator->() const { return get(); }
};

}
}

// Check implementation storage parameters
#define STATIC_VALIDATE_ALIGNED_STORAGE(T, S) \
    { \
        static_assert((decltype(S)::size() >= sizeof(T)), "StorageSize must be increased!"); \
        static_assert(((decltype(S)::align()% alignof(T)) == 0), "StorageAlign must be adjusted!"); \
    }
