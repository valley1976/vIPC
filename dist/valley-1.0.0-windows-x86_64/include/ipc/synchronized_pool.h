// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "ipc/synchronized.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ipc {

// 用于跨进程校验 slot 的 C++ 类型。
// 注意：它依赖编译器生成的类型签名，同一编译器/构建内稳定；
// 不同编译器或 ABI 不保证一致。
template<typename T>
constexpr std::uint64_t type_hash() noexcept
{
#if defined(_MSC_VER)
    constexpr const char* name = __FUNCSIG__;
#else
    constexpr const char* name = __PRETTY_FUNCTION__;
#endif
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char* p = name; *p != '\0'; ++p) {
        hash ^= static_cast<unsigned char>(*p);
        hash *= 1099511628211ULL;
    }
    return hash;
}

struct Synchronized_spec
{
    std::string name;
    std::size_t size = 0;
    std::size_t alignment = 64;
};

// 由 C++ 类型生成 slot 描述；不同 slot 可以使用不同的 T。
template<typename T>
[[nodiscard]] Synchronized_spec make_synchronized_spec(std::string name)
{
    return Synchronized_spec{
        std::move(name), Synchronized<T>::required_size(),
        Synchronized<T>::required_alignment()};
}

// 配置驱动的 Synchronized pool。
// 数量、slot 大小和对齐由配置在创建时确定，运行期不增删。
class Synchronized_pool
{
public:
    static constexpr std::size_t NAME_CAPACITY = 32;

    Synchronized_pool() = default;
    ~Synchronized_pool();
    Synchronized_pool(const Synchronized_pool&) = delete;
    Synchronized_pool& operator=(const Synchronized_pool&) = delete;
    Synchronized_pool(Synchronized_pool&& other) noexcept;
    Synchronized_pool& operator=(Synchronized_pool&& other) noexcept;

    // 仅创建者可以销毁；要求所有 Synchronized<T> 句柄已经销毁。
    // 会销毁 slot 内的 Robust_mutex，之后 slot 才可重新 create。
    void destroy() noexcept;

    [[nodiscard]] static std::size_t required_size(
        const std::vector<Synchronized_spec>& specs) noexcept;
    [[nodiscard]] static std::size_t required_alignment(
        const std::vector<Synchronized_spec>& specs) noexcept;

    // supervisor 在创建 region 后调用；只初始化 header/entries。
    [[nodiscard]] static Synchronized_pool create(
        Region_view region, const std::vector<Synchronized_spec>& specs);

    // 客户端打开已有 pool；布局由 header 自描述。
    [[nodiscard]] static Synchronized_pool open(Region_view region);

    template<typename T, typename... Args>
    Synchronized<T> create(const std::string& name, typename Synchronized<T>::Options options,
                           Args&&... args)
    {
        if (options.mutex_name.empty())
            options.mutex_name = mutex_name(name);
        Slot* slot = find(name);
        if (slot == nullptr)
            throw std::runtime_error{"Synchronized_pool::create: slot missing"};
        std::uint32_t expected = SLOT_EMPTY;
        if (!compare_exchange_state(&slot->state, expected, SLOT_INITIALIZING))
            throw std::runtime_error{"Synchronized_pool::create: slot unavailable"};
        store_type_hash(&slot->type_hash, type_hash<T>());
        if (slot->size < Synchronized<T>::required_size() ||
            slot->alignment < Synchronized<T>::required_alignment()) {
            store_state(&slot->state, SLOT_CORRUPT);
            throw std::runtime_error{
                "Synchronized_pool::create: slot too small or misaligned"};
        }

        try {
            auto object = Synchronized<T>::create_at(
                static_cast<std::byte*>(base_) + slot->offset, slot->size,
                std::move(options), std::forward<Args>(args)...);
            store_state(&slot->state, SLOT_READY);
            return object;
        } catch (...) {
            store_state(&slot->state, SLOT_CORRUPT);
            throw;
        }
    }

    template<typename T>
    [[nodiscard]] Synchronized<T> open(
        const std::string& name, typename Synchronized<T>::Options options = {}) const
    {
        if (options.mutex_name.empty())
            options.mutex_name = mutex_name(name);
        const Slot* slot = find(name);
        if (slot == nullptr || load_state(&slot->state) != SLOT_READY)
            throw std::runtime_error{"Synchronized_pool::open: slot not ready"};
        const std::uint64_t actual_type = load_type_hash(&slot->type_hash);
        if (actual_type == 0 || actual_type != type_hash<T>())
            throw std::runtime_error{"Synchronized_pool::open: slot type mismatch"};
        if (slot->size < Synchronized<T>::required_size() ||
            slot->alignment < Synchronized<T>::required_alignment()) {
            throw std::runtime_error{
                "Synchronized_pool::open: slot too small or misaligned"};
        }
        return Synchronized<T>::open_at(
            static_cast<std::byte*>(base_) + slot->offset, slot->size,
            std::move(options));
    }

    [[nodiscard]] bool valid() const noexcept { return header_ != nullptr; }
    [[nodiscard]] bool is_owner() const noexcept { return owner_; }
    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    struct Header
    {
        std::uint64_t magic = 0;
        std::uint32_t version = 0;
        std::uint32_t count = 0;
        std::uint64_t layout_hash = 0;
        std::uint64_t total_size = 0;
        char name[NAME_CAPACITY]{};
    };

    enum : std::uint32_t
    {
        SLOT_EMPTY = 0,
        SLOT_INITIALIZING = 1,
        SLOT_READY = 2,
        SLOT_CORRUPT = 3
    };

    struct Slot
    {
        char name[NAME_CAPACITY]{};
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
        std::uint64_t alignment = 0;
        std::uint64_t type_hash = 0;
        std::uint64_t mutex_offset = 0;
        std::uint32_t state = SLOT_EMPTY;
        std::uint32_t reserved = 0;
    };

    static constexpr std::uint64_t MAGIC = 0x53594E43504F4F4CULL;  // "SYNCPOOL"
    static constexpr std::uint32_t VERSION = 1;

    static std::uint64_t layout_hash(
        const std::vector<Synchronized_spec>& specs) noexcept;
    static std::size_t align_up(std::size_t value,
                                std::size_t alignment) noexcept;
    static bool valid_spec(const Synchronized_spec& spec) noexcept;
    static std::uint32_t load_state(const std::uint32_t* address) noexcept;
    static void store_state(std::uint32_t* address,
                            std::uint32_t value) noexcept;
    static bool compare_exchange_state(std::uint32_t* address,
                                       std::uint32_t& expected,
                                       std::uint32_t desired) noexcept;
    static std::uint64_t load_type_hash(
        const std::uint64_t* address) noexcept;
    static void store_type_hash(std::uint64_t* address,
                                std::uint64_t value) noexcept;

    [[nodiscard]] Slot* find(const std::string& name) noexcept;
    [[nodiscard]] const Slot* find(const std::string& name) const noexcept;

    [[nodiscard]] std::string mutex_name(const std::string& slot) const;

    std::string name_;
    void* base_ = nullptr;
    std::size_t size_ = 0;
    bool owner_ = false;
    Header* header_ = nullptr;
    Slot* slots_ = nullptr;
};

}  // namespace ipc
