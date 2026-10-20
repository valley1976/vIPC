#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>

#if defined(_WIN32)
#  include <intrin.h>
#endif

namespace valley {
namespace base {

namespace endian_detail 
{
// ============================================================
//  Compile-time endianness detection.
//
//  Resolution order (first match wins):
//    1. __BYTE_ORDER__ triplet    ！ authoritative, exposed by most
//                                   compilers (GCC, Clang, ICC, ...)
//    2. Compiler-dialect macros   ！ MSVC and other single-vendor
//                                   dialects that lack (1)
//    3. Architecture whitelist    ！ conservative fallback, *NOT*
//                                   an authoritative determination
//    4. #error                    ！ refuse to guess
// ============================================================

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)

    // ---- 1. Primary: compiler-provided ordered constants ----
#  if   __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define ENDIAN_IS_BIG_ENDIAN 1
#  elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    define ENDIAN_IS_BIG_ENDIAN 0
#  else
#    error "Unrecognized __BYTE_ORDER__ value"
#  endif

#elif defined(_MSC_VER)
    // ---- 2. Compiler dialect ----
    // MSVC exposes no __BYTE_ORDER__; every target it currently
    // supports is little-endian.
#  define ENDIAN_IS_BIG_ENDIAN 0

#elif defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#  define ENDIAN_IS_BIG_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#  define ENDIAN_IS_BIG_ENDIAN 0

    // ---- 3. Conservative architecture whitelist ----
    // NOTE: This is a *conservative assumption*, not an authoritative
    // determination.  It exists only to keep known targets building
    // on compilers that expose none of the macros above.  If your
    // platform is missing, add it here ！ but prefer fixing layer 1/2
    // first.
#elif defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64) || \
      defined(__i386__) || defined(__x86_64__) ||                                    \
      defined(__aarch64__) || defined(__ARMEL__) || defined(__ARM_EABI__) ||         \
      defined(__riscv) || defined(__riscv__) || defined(__loongarch__)
#  define ENDIAN_IS_BIG_ENDIAN 0
#elif defined(__ARMEB__) || defined(__MIPSEB__) || defined(__MIPSEB) || \
      defined(__sparc__) || defined(__s390__) || defined(__hppa__) ||   \
      defined(__powerpc__) || defined(__ppc__)
#  define ENDIAN_IS_BIG_ENDIAN 1

    // ---- 4. Refuse to guess ----
#else
#  error "Cannot determine endianness at compile time; please report your platform"
#endi

inline constexpr bool is_big = (ENDIAN_IS_BIG_ENDIAN != 0);

#undef ENDIAN_IS_BIG_ENDIAN
}

class Endian
{
public:
    Endian() = delete;
    Endian(const Endian&) = delete;
    Endian(Endian&&) = delete;
    ~Endian() = delete;

    Endian& operator=(const Endian&) = delete;
    Endian& operator=(Endian&&) = delete;

    //! Is big-endian system?
    static constexpr bool is_be() noexcept { return endian_detail::is_big; }
    //! Is little-endian system?
    static constexpr bool is_le() noexcept { return !endian_detail::is_big; }

    //! Read big-endian signed 16-bit value
    static size_t read_be(const void* buffer, int16_t& value);
    static size_t read_be(const void* buffer, uint16_t& value);
    static size_t read_be(const void* buffer, int32_t& value);
    static size_t read_be(const void* buffer, uint32_t& value);
    static size_t read_be(const void* buffer, int64_t& value);
    static size_t read_be(const void* buffer, uint64_t& value);

    //! Read little-endian signed 16-bit value
    static size_t read_le(const void* buffer, int16_t& value);
    static size_t read_le(const void* buffer, uint16_t& value);
    static size_t read_le(const void* buffer, int32_t& value);
    static size_t read_le(const void* buffer, uint32_t& value);
    static size_t read_le(const void* buffer, int64_t& value);
    static size_t read_le(const void* buffer, uint64_t& value);

    //! Write big-endian value
    static size_t write_be(void* buffer, int16_t value);
    static size_t write_be(void* buffer, uint16_t value);
    static size_t write_be(void* buffer, int32_t value);
    static size_t write_be(void* buffer, uint32_t value);
    static size_t write_be(void* buffer, int64_t value);
    static size_t write_be(void* buffer, uint64_t value);

    //! Write little-endian value
    static size_t write_le(void* buffer, int16_t value);
    static size_t write_le(void* buffer, uint16_t value);
    static size_t write_le(void* buffer, int32_t value);
    static size_t write_le(void* buffer, uint32_t value);
    static size_t write_le(void* buffer, int64_t value);
    static size_t write_le(void* buffer, uint64_t value);

private:
    static uint8_t  swap_unsigned(uint8_t  v) { return v; }

    static uint16_t swap_unsigned(uint16_t v)
    {
#if defined(_WIN32)
        return _byteswap_ushort(v);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap16(v);
#else
        return static_cast<uint16_t>((v >> 8) | (v << 8));
#endif
    }

    static uint32_t swap_unsigned(uint32_t v)
    {
#if defined(_WIN32)
        return _byteswap_ulong(v);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(v);
#else
        return ((v & 0x000000FFu) << 24) |
            ((v & 0x0000FF00u) << 8) |
            ((v & 0x00FF0000u) >> 8) |
            ((v & 0xFF000000u) >> 24);
#endif
    }

    static uint64_t swap_unsigned(uint64_t v)
    {
#if defined(_WIN32)
        return _byteswap_uint64(v);
#elif defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(v);
#else
        return ((v & 0x00000000000000FFull) << 56) |
            ((v & 0x000000000000FF00ull) << 40) |
            ((v & 0x0000000000FF0000ull) << 24) |
            ((v & 0x00000000FF000000ull) << 8) |
            ((v & 0x000000FF00000000ull) >> 8) |
            ((v & 0x0000FF0000000000ull) >> 24) |
            ((v & 0x00FF000000000000ull) >> 40) |
            ((v & 0xFF00000000000000ull) >> 56);
#endif
    }

    template <typename T>
    static T byte_swap(T v)
    {
        static_assert(std::is_integral<T>::value, "T must be integral");
        using U = typename std::make_unsigned<T>::type;
        U u;
        std::memcpy(&u, &v, sizeof(T));
        u = Endian::swap_unsigned(u);
        std::memcpy(&v, &u, sizeof(T));
        return v;
    }

    template <typename T>
    static size_t read_be_impl(const void* buffer, T& value)
    {
        T tmp;
        std::memcpy(&tmp, buffer, sizeof(T));
        value = is_be() ? tmp : byte_swap(tmp);
        return sizeof(T);
    }

    template <typename T>
    static size_t read_le_impl(const void* buffer, T& value)
    {
        T tmp;
        std::memcpy(&tmp, buffer, sizeof(T));
        value = is_le() ? tmp : byte_swap(tmp);
        return sizeof(T);
    }

    template <typename T>
    static size_t write_be_impl(void* buffer, T value)
    {
        T tmp = is_be() ? value : byte_swap(value);
        std::memcpy(buffer, &tmp, sizeof(T));
        return sizeof(T);
    }

    template <typename T>
    static size_t write_le_impl(void* buffer, T value)
    {
        T tmp = is_le() ? value : byte_swap(value);
        std::memcpy(buffer, &tmp, sizeof(T));
        return sizeof(T);
    }
};

// ============ read_be ============
inline size_t Endian::read_be(const void* b, int16_t& v) { return read_be_impl(b, v); }
inline size_t Endian::read_be(const void* b, uint16_t& v) { return read_be_impl(b, v); }
inline size_t Endian::read_be(const void* b, int32_t& v) { return read_be_impl(b, v); }
inline size_t Endian::read_be(const void* b, uint32_t& v) { return read_be_impl(b, v); }
inline size_t Endian::read_be(const void* b, int64_t& v) { return read_be_impl(b, v); }
inline size_t Endian::read_be(const void* b, uint64_t& v) { return read_be_impl(b, v); }

// ============ read_le ============
inline size_t Endian::read_le(const void* b, int16_t& v) { return read_le_impl(b, v); }
inline size_t Endian::read_le(const void* b, uint16_t& v) { return read_le_impl(b, v); }
inline size_t Endian::read_le(const void* b, int32_t& v) { return read_le_impl(b, v); }
inline size_t Endian::read_le(const void* b, uint32_t& v) { return read_le_impl(b, v); }
inline size_t Endian::read_le(const void* b, int64_t& v) { return read_le_impl(b, v); }
inline size_t Endian::read_le(const void* b, uint64_t& v) { return read_le_impl(b, v); }

// ============ write_be ============
inline size_t Endian::write_be(void* b, int16_t  v) { return write_be_impl(b, v); }
inline size_t Endian::write_be(void* b, uint16_t v) { return write_be_impl(b, v); }
inline size_t Endian::write_be(void* b, int32_t  v) { return write_be_impl(b, v); }
inline size_t Endian::write_be(void* b, uint32_t v) { return write_be_impl(b, v); }
inline size_t Endian::write_be(void* b, int64_t  v) { return write_be_impl(b, v); }
inline size_t Endian::write_be(void* b, uint64_t v) { return write_be_impl(b, v); }

// ============ write_le ============
inline size_t Endian::write_le(void* b, int16_t  v) { return write_le_impl(b, v); }
inline size_t Endian::write_le(void* b, uint16_t v) { return write_le_impl(b, v); }
inline size_t Endian::write_le(void* b, int32_t  v) { return write_le_impl(b, v); }
inline size_t Endian::write_le(void* b, uint32_t v) { return write_le_impl(b, v); }
inline size_t Endian::write_le(void* b, int64_t  v) { return write_le_impl(b, v); }
inline size_t Endian::write_le(void* b, uint64_t v) { return write_le_impl(b, v); }

}
}