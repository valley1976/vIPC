// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 大块具名共享内存 + 用户命名的子区域。
//
//   shm::Shared_memory::create("jobs", {{"control", 4096}, {"data", 1 << 20}});
//
// 每个子区域前、后各有一个保护页（POSIX PROT_NONE / Windows PAGE_NOACCESS），
// 越界访问会立刻触发访问异常；子区域数据按系统页对齐。
//
// 布局（全部按页对齐）：
//   Header + Entry[] | guard | region0 | guard | guard | region1 | guard | ...
// 每个 region 前后各一页 guard，保证任意方向越界都被隔离。
//
// 只支持 Windows 与 Linux；实现位于 src/shared_memory.cpp。

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <vector>



namespace shm {

struct Region_spec
{
    std::string name;
    std::size_t size = 0;
};

struct Region_view
{
    std::string name;
    void* data = nullptr;
    std::size_t size = 0;
};

class Shared_memory
{
public:
    static constexpr std::size_t MAX_REGIONS = 32;
    static constexpr std::size_t NAME_CAPACITY = 32;

    Shared_memory() = default;
    ~Shared_memory();

    Shared_memory(Shared_memory&& other) noexcept;
    Shared_memory& operator=(Shared_memory&& other) noexcept;

    Shared_memory(const Shared_memory&)            = delete;
    Shared_memory& operator=(const Shared_memory&) = delete;

    // region 名称必须非空、长度 < NAME_CAPACITY、全局唯一；size 必须 > 0。
    [[nodiscard]] static Shared_memory create(const std::string& name,
                                              const std::vector<Region_spec>& regions,
                                              bool replace_stale = false,
                                              std::error_code* ec = nullptr);
    [[nodiscard]] static Shared_memory open(const std::string& name,
                                            std::error_code* ec = nullptr);

    // 按当前系统页大小计算 create() 需要的总映射大小；无效输入返回 0。
    [[nodiscard]] static std::size_t required_size(
        const std::vector<Region_spec>& regions);

    [[nodiscard]] bool is_open() const noexcept { return base_ != nullptr; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] std::size_t total_size() const noexcept { return total_; }
    [[nodiscard]] std::size_t page_size() const noexcept { return page_; }
    [[nodiscard]] std::size_t region_count() const noexcept { return count_; }

    [[nodiscard]] Region_view region(std::size_t index) const;
    [[nodiscard]] Region_view find(const std::string& name) const;

    // 保护页地址；越界探测/测试用
    [[nodiscard]] void* guard_before(std::size_t index) const;
    [[nodiscard]] void* guard_after(std::size_t index) const;

private:
    struct Header;
    struct Entry;

    static std::size_t system_page_size() noexcept;

    [[nodiscard]] Header* header() const noexcept;
    [[nodiscard]] Entry* entries() const noexcept;
    [[nodiscard]] bool protect_guards() noexcept;
    void reset() noexcept;
    void destroy_mapping() noexcept;

#if defined(_WIN32)
    void* mapping_ = nullptr;
#else
    int fd_ = -1;
#endif
    std::string name_;
    void* base_ = nullptr;
    std::size_t total_ = 0;
    std::size_t mapped_size_ = 0;
    std::size_t page_ = 0;
    std::size_t count_ = 0;
    bool owner_ = false;
};

}  // namespace shm
