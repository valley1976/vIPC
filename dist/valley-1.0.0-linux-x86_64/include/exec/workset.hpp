// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "exec/export.h"

// Workset：放在一块调用方提供的内存里的任务集 + 2-bit 状态机。
//   - 调用方负责内存（mmap / 共享内存 / 静态缓冲），本类不持有、不释放；
//   - 同一块内存可以被 create() 初始化一次，之后用 open() 重新附着；
//   - 任务通过 register_task() 按事件下标或事件名注册；
//   - schedule() 只做状态迁移，真正的执行由 poll()/poll_wait() 驱动。
//
// 公开头只依赖标准库；Signals 等实现细节在 exec/src/ 下。

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace ipc {
class Notification;
}

namespace exec {

// 跳过任务/内部错误的上报信息；message 指向静态字符串，生命周期由 Workset 保证。
struct Error_info
{
    std::size_t task_index = 0;
    std::error_code code{};
    const char* message = nullptr;
};

// 在 poll() 内同步调用；为空表示不上报
using Error_reporter = std::function<void(const Error_info&)>;

class LIBVALLEY_EXEC_EXPORT Workset
{
public:
    static constexpr std::size_t CACHE_LINE_SIZE = 64;
    static constexpr std::size_t MAX_CONSUMERS = 8;

    struct Consumer_stats
    {
        std::uint64_t executed = 0;
        std::uint64_t skipped = 0;
    };

    using Task = std::function<void()>;

    // 为事件/task 下标注册回调；空 Task 表示清空该槽位。
    // 建议在 poll 前完成注册；实现内部用 mutex 保护并发注册。
    [[nodiscard]] bool register_task(std::size_t task_index, Task task);

    // 通过事件名注册；需要先 bind_notification() 且该 group 已 Event::bind()。
    [[nodiscard]] bool register_task(const std::string& event_name, Task task);

    Workset();
    ~Workset();

    Workset(Workset&&) noexcept;
    Workset& operator=(Workset&&) noexcept;

    Workset(const Workset&)            = delete;
    Workset& operator=(const Workset&) = delete;

    // 需要的总字节数（header + signals + work[]，全部按 cache line 对齐）
    [[nodiscard]] static std::size_t calculate_memory_size(std::size_t task_count);

    // 在 ptr/size 上原地初始化；参数非法或空间不足时返回无效对象（max_task() == 0）
    [[nodiscard]] static Workset create(void* ptr, std::size_t size, std::size_t task_count);
    // 同上，失败原因写到 ec；成功时 ec.clear()
    [[nodiscard]] static Workset create(void* ptr, std::size_t size, std::size_t task_count,
                                        std::error_code& ec);

    // 附着到已经 create 过的内存；magic/version 不匹配时返回无效对象
    [[nodiscard]] static Workset open(void* ptr);
    [[nodiscard]] static Workset open(void* ptr, std::error_code& ec);
    // 带映射大小：size 非 0 时额外校验 size >= Header::total_size
    [[nodiscard]] static Workset open(void* ptr, std::size_t size);
    [[nodiscard]] static Workset open(void* ptr, std::size_t size, std::error_code& ec);

    [[nodiscard]] std::size_t max_task() const noexcept;
    [[nodiscard]] bool is_open() const noexcept;

    // 绑定 supervisor 创建的 notification；schedule 同步 set，poll_wait 使用 wait。
    // notification 生命周期由调用方保证，必须在 Workset 使用期间有效。
    void bind_notification(ipc::Notification* notification) noexcept;
    [[nodiscard]] ipc::Notification* notification() const noexcept;

    // 可选：空 task 被跳过时回调；只作用于当前 Workset 对象，不进入共享内存
    void set_error_reporter(Error_reporter reporter);
    [[nodiscard]] Error_reporter error_reporter() const;

    // {是否接受, 是否已经处于待执行/执行中}；越界或未初始化返回 {false,false}
    std::pair<bool, bool> schedule(std::size_t task_index) noexcept;

    // 空的 std::function 会被跳过：状态回到 IDLE，计入 skipped。
    int poll(std::size_t max_count = std::numeric_limits<std::size_t>::max());
    int poll(std::size_t max_count, std::size_t& skipped);
    // 指定 consumer 槽位（0..MAX_CONSUMERS-1）时会同时累计该消费者的执行/跳过数。
    int poll(std::size_t max_count, std::size_t consumer_id,
             std::size_t& skipped);
    // 绑定 notification 后的等待入口：poll，没有事件时 wait，再 poll。
    int poll_wait(std::chrono::milliseconds timeout,
                  std::size_t max_count = std::numeric_limits<std::size_t>::max());

    // 公平性统计：每个 consumer 槽位的累计执行/跳过数，以及每个 task 的执行次数
    [[nodiscard]] Consumer_stats consumer_stats(std::size_t consumer_id) const noexcept;
    [[nodiscard]] std::uint64_t task_runs(std::size_t task_index) const noexcept;

private:
    struct Impl;

    std::pair<bool, bool> schedule_internal(std::size_t task_index) noexcept;

    std::unique_ptr<Impl> impl_;
};

}  // namespace exec
