// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "serve/export.h"

// 单次定时器。expire_* 可在任意线程调用（内部投递到 loop 线程），
// 回调也在 loop 线程触发：参数 true = 到期，false = 被取消或覆盖。

#include <chrono>
#include <functional>
#include <memory>

namespace serve {

class Event_loop;

class LIBVALLEY_SERVE_EXPORT Steady_timer
{
public:
    explicit Steady_timer(Event_loop& loop);
    ~Steady_timer();

    Steady_timer(const Steady_timer&)            = delete;
    Steady_timer& operator=(const Steady_timer&) = delete;
    Steady_timer(Steady_timer&&)                 = delete;
    Steady_timer& operator=(Steady_timer&&)      = delete;

    void expire_after(std::function<void(bool)> action, std::chrono::milliseconds span);
    void expire_at(std::function<void(bool)> action, std::chrono::steady_clock::time_point tp);

    void cancel();
    [[nodiscard]] bool is_pending() const noexcept;

private:
    struct State;

    static void arm(const std::shared_ptr<State>& state, std::chrono::steady_clock::time_point tp,
                    std::function<void(bool)> action);

    Event_loop* loop_;
    std::shared_ptr<State> state_;
};

}  // namespace serve
