#pragma once

// Workset：放在一块调用方提供的内存里的任务集 + 2-bit 状态机。
//   - 调用方负责内存（mmap / 共享内存 / 静态缓冲），本类不持有、不释放；
//   - 同一块内存可以被 create() 初始化一次，之后用 open() 重新附着；
//   - schedule() 只做状态迁移，真正的执行由 poll()/poll_wait() 驱动。
//
// 公开头只依赖标准库 + Aligned_allocator；Signals、Wake_up 实现等细节在 exec/src/ 下。

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "exec/aligned_allocator.hpp"

namespace exec {

// 手动复位事件：set() 置位（幂等），reset() 清位，wait_for() 等到置位。
// 语义是「可能有任务待处理」，不累计次数；多个消费者可以同时被唤醒。
class Wake_up
{
public:
    virtual ~Wake_up() = default;

    virtual void set() noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual bool wait_for(std::chrono::nanoseconds timeout) noexcept = 0;
};

// 进程内事件
[[nodiscard]] std::unique_ptr<Wake_up> make_wake_up();
// 跨进程具名事件（Windows manual-reset event；POSIX 共享内存 + pshared cond）
[[nodiscard]] std::unique_ptr<Wake_up> create_shared_wake_up(const std::string& name,
                                                             bool replace_stale = false);
[[nodiscard]] std::unique_ptr<Wake_up> create_shared_wake_up(const std::string& name,
                                                             std::error_code& ec);
[[nodiscard]] std::unique_ptr<Wake_up> create_shared_wake_up(const std::string& name,
                                                             bool replace_stale,
                                                             std::error_code& ec);
[[nodiscard]] std::unique_ptr<Wake_up> open_shared_wake_up(const std::string& name);
[[nodiscard]] std::unique_ptr<Wake_up> open_shared_wake_up(const std::string& name,
                                                           std::error_code& ec);

// 跳过任务/内部错误的上报信息；message 指向静态字符串，生命周期由 Workset 保证。
struct Error_info
{
    std::size_t task_index = 0;
    std::error_code code{};
    const char* message = nullptr;
};

// 在 poll() 内同步调用；为空表示不上报
using Error_reporter = std::function<void(const Error_info&)>;

class Workset
{
public:
    static constexpr std::size_t CACHE_LINE_SIZE = 64;
    static constexpr std::size_t MAX_CONSUMERS = 8;

    struct Consumer_stats
    {
        std::uint64_t executed = 0;
        std::uint64_t skipped = 0;
    };

    using Task  = std::function<void()>;
    using Tasks = std::vector<Task, Aligned_allocator<Task, CACHE_LINE_SIZE>>;

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

    // 可选：空 task 被跳过时回调；只作用于当前 Workset 对象，不进入共享内存
    void set_error_reporter(Error_reporter reporter);
    [[nodiscard]] Error_reporter error_reporter() const;

    // {是否接受, 是否已经处于待执行/执行中}；越界或未初始化返回 {false,false}
    std::pair<bool, bool> schedule(std::size_t task_index) noexcept;

    // 首次调度成功时 set 一次，供 poll_wait 唤醒
    void schedule(Wake_up& wakeup, std::size_t task_index) noexcept;

    // 批量调度 [begin, begin + count)，只 set 一次事件；返回接受的 task 数
    std::size_t schedule_range(Wake_up& wakeup, std::size_t begin, std::size_t count) noexcept;

    // 空的 std::function 会被跳过：状态回到 IDLE，信号位已消费，计入 skipped
    int poll(const Tasks& tasks,
             std::size_t max_count = std::numeric_limits<std::size_t>::max());
    int poll(const Tasks& tasks, std::size_t max_count, std::size_t& skipped);
    // 指定 consumer 槽位（0..MAX_CONSUMERS-1）时会同时累计该消费者的执行/跳过数
    int poll(const Tasks& tasks, std::size_t max_count, std::size_t consumer_id,
             std::size_t& skipped);
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count = std::numeric_limits<std::size_t>::max());
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count, std::size_t& skipped);
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count, std::chrono::nanoseconds& waited_time);
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count, std::chrono::nanoseconds& waited_time,
                  std::size_t& skipped);
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count, std::size_t consumer_id, std::size_t& skipped);
    int poll_wait(Wake_up& wakeup, const Tasks& tasks, std::chrono::milliseconds timeout,
                  std::size_t max_count, std::size_t consumer_id,
                  std::chrono::nanoseconds& waited_time, std::size_t& skipped);

    // 公平性统计：每个 consumer 槽位的累计执行/跳过数，以及每个 task 的执行次数
    [[nodiscard]] Consumer_stats consumer_stats(std::size_t consumer_id) const noexcept;
    [[nodiscard]] std::uint64_t task_runs(std::size_t task_index) const noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}  // namespace exec
