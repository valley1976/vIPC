// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

namespace time_ {

using Utc_clock = std::chrono::system_clock;
using Utc_time =
    std::chrono::time_point<Utc_clock, std::chrono::nanoseconds>;
using Steady_time = std::chrono::steady_clock::time_point;

struct Time_sync_status
{
    bool synchronized = false;
    std::int64_t offset_ns = 0;
    std::uint64_t max_error_ns = 0;
    std::string source;
};

struct Session_anchor
{
    Utc_time utc{};
    Steady_time steady{};
};

// 可注入时钟入口；空函数指针表示使用系统时钟。
struct Clock_source
{
    using Utc_now = Utc_time (*)(void*) noexcept;
    using Steady_now = Steady_time (*)(void*) noexcept;

    Utc_now utc_now = nullptr;
    Steady_now steady_now = nullptr;
    void* context = nullptr;
};

// 一个 session 内 UTC 与 monotonic 时间的映射器。
// UTC 可能跳变；steady 只用于本进程时长。
class Session_clock
{
public:
    explicit Session_clock(
        std::int64_t jump_threshold_ns = 100'000'000) noexcept;

    // 测试/嵌入入口：所有读时钟都走 source；source 必须比 clock 活得久。
    explicit Session_clock(Clock_source source,
                           std::int64_t jump_threshold_ns = 100'000'000) noexcept;

    // 测试/显式锚点入口。
    Session_clock(Utc_time utc, Steady_time steady,
                  std::int64_t jump_threshold_ns = 100'000'000) noexcept;

    [[nodiscard]] Utc_time utc_now() const noexcept;
    [[nodiscard]] Steady_time steady_now() const noexcept;

    // 用锚点和同步偏移推导 UTC，不跟随系统时钟跳变。
    [[nodiscard]] Utc_time utc_from_steady(Steady_time time) const noexcept;

    // 由外部时间同步服务调用；offset 是 UTC 相对 session 锚点的修正量。
    // max_adjust_ns > 0 时按上限逐步调整，避免一次性 UTC 跳变。
    void update_sync(const Time_sync_status& status,
                     std::int64_t max_adjust_ns = 0) noexcept;

    // 检测当前 UTC 是否相对上次采样发生跳变。
    [[nodiscard]] bool check_clock() noexcept;

    // 测试/显式采样入口。
    [[nodiscard]] bool check_clock(Utc_time utc, Steady_time steady) noexcept;

    [[nodiscard]] bool clock_jumped() const noexcept;
    [[nodiscard]] std::uint64_t jump_count() const noexcept;
    [[nodiscard]] Time_sync_status sync_status() const;
    [[nodiscard]] Session_anchor anchor() const noexcept;

private:
    Clock_source source_{};
    Session_anchor anchor_;
    std::int64_t jump_threshold_ns_;

    std::atomic<std::int64_t> offset_ns_{0};
    std::atomic<std::uint64_t> max_error_ns_{0};
    std::atomic<std::uint64_t> jump_count_{0};
    std::atomic<bool> synchronized_{false};
    std::atomic<bool> has_last_error_{false};
    std::atomic<std::int64_t> last_raw_error_ns_{0};

    mutable std::mutex sync_mutex_;
    std::string sync_source_;
};

// 固定 UTC 格式，默认 YYYYMMDDTHHMMSSZ。
[[nodiscard]] std::string
format_utc(Utc_time time, const char* format = "%Y%m%dT%H%M%SZ");

}  // namespace time_
