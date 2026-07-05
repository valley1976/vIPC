#pragma once

#include <cstddef>
#include <chrono>
#include <functional>
#include <memory>

#include <valley/lang/storage.h>

namespace valley {
namespace thread {

using Milliseconds = std::chrono::milliseconds;

class Workset
{
public:
    using Task = std::function<void()>;

    explicit Workset(std::initializer_list<Task> tasks);
    ~Workset();

    Workset(const Workset&) = delete;
    Workset& operator=(const Workset&) = delete;

    size_t max_task() const;

    void schedule(size_t task_index);

    int poll();
    int poll_wait(const Milliseconds& timeout = Milliseconds::max());

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}