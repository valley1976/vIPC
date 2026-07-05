#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <functional>

namespace valley {
namespace exec {

using Milliseconds = std::chrono::milliseconds;
using Task = std::function<void()>;

class Executor
{
public:
    using Named_task = std::pair<std::string, Task>;

    Executor() = default;
    explicit Executor(const std::string& name, std::initializer_list<Named_task> task);
    ~Executor();

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    Executor(Executor&& orig) noexcept;
    Executor& operator=(Executor&& orig) noexcept;

    bool is_valid() const { return impl_ != nullptr; }

    size_t max_task() const;

    void schedule(size_t task_index);

    int poll();
    int poll_wait(const Milliseconds& timeout = Milliseconds::max());

    void run(std::atomic<bool>& stop_flag);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}