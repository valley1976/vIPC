#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept> 

namespace valley {
namespace lang {

template <class T, size_t kAlign = alignof(T)>
struct Aligned_allocator
{
    static_assert(kAlign % alignof(T) == 0, "bad align");

    using value_type = T;
    using pointer = T*;

    template<typename U>
    struct rebind {
        using other = Aligned_allocator<U, kAlign>;
    };

    Aligned_allocator() = default;

    template <typename U>
    Aligned_allocator(Aligned_allocator<U, kAlign> const& rhs) {}

    pointer allocate(std::size_t n)
    {
        if (0 == n) return nullptr;

#ifdef _WIN32
        auto* ptr = _aligned_malloc(n * sizeof(T), kAlign);
#else
        auto* ptr = aligned_alloc(kAlign, n * sizeof(T));
#endif // _WIN32
        if (!ptr) throw std::bad_alloc();

        return reinterpret_cast<pointer>(ptr);
    }

    void deallocate(pointer p, std::size_t n)
    {
#ifdef _WIN32
        if (p) _aligned_free(p);
#else
        free(p);
#endif // _WIN32
    }

    template <typename U>
    bool operator==(Aligned_allocator<U, kAlign> const& rhs) const
    {
        return true;
    }

    template <typename U>
    bool operator!=(Aligned_allocator<U, kAlign> const& rhs) const
    {
        return false;
    }
};

} // namespace lang
} // namespace valley