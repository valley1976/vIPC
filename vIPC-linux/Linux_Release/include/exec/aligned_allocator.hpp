#pragma once

// 按指定对齐分配 T 的 allocator；给 std::vector 的存储指定边界。
// 例：std::vector<Task, Aligned_allocator<Task, 64>>。

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

#if defined(_WIN32)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace exec {

template <class T, std::size_t ALIGN = alignof(T)>
struct Aligned_allocator
{
    static_assert((ALIGN & (ALIGN - 1)) == 0, "ALIGN 必须是 2 的幂");
    static_assert(ALIGN % alignof(T) == 0, "ALIGN 必须是 alignof(T) 的整数倍");

    using value_type = T;
    using pointer    = T*;

    template <typename U>
    struct rebind
    {
        using other = Aligned_allocator<U, ALIGN>;
    };

    Aligned_allocator() = default;

    template <typename U>
    Aligned_allocator(const Aligned_allocator<U, ALIGN>&) noexcept
    {
    }

    [[nodiscard]] pointer allocate(std::size_t n)
    {
        if (n == 0)
            return nullptr;

        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        // POSIX 的 aligned_alloc 要求 size 是 alignment 的整数倍；
        // 统一向上取整，Windows 的 _aligned_malloc 也接受这个尺寸。
        const std::size_t bytes = n * sizeof(T);
        if (bytes > std::numeric_limits<std::size_t>::max() - (ALIGN - 1))
            throw std::bad_array_new_length();

        const std::size_t rounded = (bytes + ALIGN - 1) / ALIGN * ALIGN;

#if defined(_WIN32)
        void* ptr = _aligned_malloc(rounded, ALIGN);
#else
        void* ptr = std::aligned_alloc(ALIGN, rounded);
#endif
        if (ptr == nullptr)
            throw std::bad_alloc();

        return static_cast<pointer>(ptr);
    }

    void deallocate(pointer ptr, std::size_t) noexcept
    {
#if defined(_WIN32)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }

    template <typename U>
    bool operator==(const Aligned_allocator<U, ALIGN>&) const noexcept
    {
        return true;
    }

    template <typename U>
    bool operator!=(const Aligned_allocator<U, ALIGN>&) const noexcept
    {
        return false;
    }
};

}  // namespace exec
