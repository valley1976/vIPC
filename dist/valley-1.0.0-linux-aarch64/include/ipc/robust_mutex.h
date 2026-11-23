// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstddef>
#include <string>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <pthread.h>
#endif

namespace ipc {

// 跨进程 robust mutex。
// POSIX 直接放在共享内存；Windows 只保存命名 mutex 的名字，
// 真正的 HANDLE 由每个进程在本地打开并缓存。
class Robust_mutex
{
public:
    enum class Lock_state
    {
        ACQUIRED,
        RECOVERED
    };

    explicit Robust_mutex(const std::string& name = {});
    ~Robust_mutex();

    Robust_mutex(const Robust_mutex&) = delete;
    Robust_mutex& operator=(const Robust_mutex&) = delete;
    Robust_mutex(Robust_mutex&&) = delete;
    Robust_mutex& operator=(Robust_mutex&&) = delete;

    [[nodiscard]] Lock_state lock();
    [[nodiscard]] bool try_lock(Lock_state& state);
    void unlock() noexcept;

private:
#if defined(_WIN32)
    static constexpr std::size_t NAME_CAPACITY = 96;
    char name_[NAME_CAPACITY]{};
#else
    pthread_mutex_t mutex_{};
#endif
};

}  // namespace ipc
