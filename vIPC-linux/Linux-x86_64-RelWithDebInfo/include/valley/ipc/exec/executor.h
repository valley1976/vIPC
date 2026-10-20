#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <functional>
#include <vector>

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {

using Milliseconds = std::chrono::milliseconds;
using Task = std::function<void()>;

class LIBVALLEY_IPC_EXPORT Executor
{
public:
    using Named_task = std::pair<std::string, Task>;

    Executor();
    ~Executor();

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    Executor(Executor&& orig) noexcept;
    Executor& operator=(Executor&& orig) noexcept;

    static Executor create(const std::string& name, std::initializer_list<Named_task> task);
    static Executor create(const std::string& name, const std::vector<Named_task>& task);

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    void schedule(size_t task_index);

    int poll(size_t max_count = std::numeric_limits<size_t>::max());
    int poll_wait(const Milliseconds& timeout = Milliseconds::max(), size_t max_count = std::numeric_limits<size_t>::max());
    int poll_wait(const Milliseconds& timeout, size_t max_count, std::chrono::nanoseconds& waited_time);

    void run(std::atomic<bool>& stop_flag);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}