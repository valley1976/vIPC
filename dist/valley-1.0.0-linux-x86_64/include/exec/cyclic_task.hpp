// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 周期任务循环：严格周期 + 停止 + 统计。
//
//   exec::Cyclic_task task(cfg, task_fn);
//   task.run();       // 阻塞运行，直到 stop() 或任务抛异常
//
// 公开头只依赖标准库；周期定时器、Signals 等实现细节在 exec/src/ 下。

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace exec {

class Cyclic_task
{
public:
    struct Config
    {
        std::chrono::nanoseconds period{std::chrono::milliseconds(10)};
        // nullopt = 首次唤醒时刻作为网格起点
        std::optional<std::int64_t> epoch_ns{std::nullopt};
        std::string                 name;
        // 尾部自旋窗口（纳秒）：睡眠误差被限制在该窗口内，0 = 纯睡眠
        std::int64_t                spin_ns{0};
        // 在 run() 开头调用一次（绑核、SCHED_FIFO、mlockall 等）
        std::function<void()>       thread_setup;
    };

    struct Stats
    {
        static constexpr std::size_t HISTOGRAM_BINS = 32;

        std::uint64_t ticks     = 0;   // 已执行周期数
        std::uint64_t misses    = 0;   // 累计错失相位
        std::int64_t  wake_max  = 0;   // 最大唤醒偏差（ns）
        std::int64_t  exec_max  = 0;   // 最大执行时间（ns）
        std::int64_t  exec_last = 0;   // 上次执行时间（ns）

        // 唤醒偏差直方图：bucket i 覆盖 [2^(i-1), 2^i) ns，bucket 0 为 < 1ns
        std::array<std::uint64_t, HISTOGRAM_BINS> wake_hist{};

        // 返回第 percentile 百分位的上界（ns）；没有样本时返回 0
        [[nodiscard]] std::uint64_t wake_percentile(double percentile) const noexcept
        {
            std::uint64_t total = 0;
            for (const auto count : wake_hist)
                total += count;
            if (total == 0)
                return 0;

            if (percentile <= 0.0)
                return 0;
            if (percentile > 100.0)
                percentile = 100.0;

            std::uint64_t target =
                static_cast<std::uint64_t>(percentile / 100.0 * static_cast<double>(total));
            if (target == 0)
                target = 1;

            std::uint64_t cumulative = 0;
            for (std::size_t i = 0; i < HISTOGRAM_BINS; ++i) {
                cumulative += wake_hist[i];
                if (cumulative >= target)
                    return i == 0 ? 0 : (std::uint64_t{1} << i);
            }
            return std::uint64_t{1} << (HISTOGRAM_BINS - 1);
        }

        void print(FILE* f = stdout, const char* tag = "") const
        {
            std::fprintf(f, "%sticks=%llu misses=%llu wake_max=%lld exec_max=%lld exec_last=%lld\n",
                         tag,
                         static_cast<unsigned long long>(ticks),
                         static_cast<unsigned long long>(misses),
                         static_cast<long long>(wake_max),
                         static_cast<long long>(exec_max),
                         static_cast<long long>(exec_last));
        }
    };

    using Task = std::function<void()>;

    Cyclic_task(Config cfg, Task task);
    ~Cyclic_task();

    // 接受任意可调用对象，内部统一为 std::function
    template <typename F,
              typename = std::enable_if_t<
                  std::is_invocable_r_v<void, F&> &&
                  !std::is_same_v<std::decay_t<F>, Cyclic_task> &&
                  !std::is_same_v<std::decay_t<F>, Task>>>
    Cyclic_task(Config cfg, F&& f)
        : Cyclic_task(std::move(cfg), Task(std::forward<F>(f)))
    {
    }

    Cyclic_task(const Cyclic_task&)            = delete;
    Cyclic_task& operator=(const Cyclic_task&) = delete;
    Cyclic_task(Cyclic_task&&)                 = delete;
    Cyclic_task& operator=(Cyclic_task&&)      = delete;

    // 阻塞运行直到 stop() 或任务抛异常。不能在同一个对象上重复调用。
    void run();

    // 请求停止；run() 最坏延迟一个周期后返回
    void stop() noexcept { stop_.store(true, std::memory_order_release); }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        return stop_.load(std::memory_order_acquire);
    }

    // 从任意线程安全读取
    [[nodiscard]] Stats stats() const;

    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] const std::string& name() const noexcept { return config_.name; }
    [[nodiscard]] std::chrono::nanoseconds period() const noexcept { return config_.period; }

private:
    struct Impl;

    Config            config_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> ran_{false};
    std::unique_ptr<Impl> impl_;
};

}  // namespace exec
