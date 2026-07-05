#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace valley {
namespace sync {

class Shared_mutex {
public:
    Shared_mutex() noexcept
    {
#if defined(_WIN32) || defined(_WIN64)
        InitializeSRWLock(&lock_);
#else
        pthread_rwlock_init(&lock_, nullptr);
#endif
    }

    ~Shared_mutex()
    {
#if !(defined(_WIN32) || defined(_WIN64))
        pthread_rwlock_destroy(&lock_);
#endif
    }

    // 独占锁（写）
    void lock()
    {
#if defined(_WIN32) || defined(_WIN64)
        AcquireSRWLockExclusive(&lock_);
#else
        pthread_rwlock_wrlock(&lock_);
#endif
    }

    void unlock()
    {
#if defined(_WIN32) || defined(_WIN64)
        ReleaseSRWLockExclusive(&lock_);
#else
        pthread_rwlock_unlock(&lock_);
#endif
    }

    // 共享锁（读）
    void lock_shared()
    {
#if defined(_WIN32) || defined(_WIN64)
        AcquireSRWLockShared(&lock_);
#else
        pthread_rwlock_rdlock(&lock_);
#endif
    }

    void unlock_shared()
    {
#if defined(_WIN32) || defined(_WIN64)
        ReleaseSRWLockShared(&lock_);
#else
        pthread_rwlock_unlock(&lock_);
#endif
    }

    Shared_mutex(const Shared_mutex&) = delete;
    Shared_mutex& operator=(const Shared_mutex&) = delete;

private:
#if defined(_WIN32) || defined(_WIN64)
    SRWLOCK lock_;
#else
    pthread_rwlock_t lock_;
#endif
};

class Shared_lock {
public:
    explicit Shared_lock(Shared_mutex& mtx) : mtx_(mtx)
    {
        mtx_.lock_shared();
    }

    ~Shared_lock()
    {
        mtx_.unlock_shared();
    }

    Shared_lock(const Shared_lock&) = delete;
    Shared_lock& operator=(const Shared_lock&) = delete;
private:
    Shared_mutex& mtx_;
};

}
}
