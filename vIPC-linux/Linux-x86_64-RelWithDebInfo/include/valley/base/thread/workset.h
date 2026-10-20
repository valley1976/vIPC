#pragma once

#include <cstddef>
#include <chrono>
#include <functional>
#include <memory>

#include "valley/base/export.h"

namespace valley {
namespace base {

using Milliseconds = std::chrono::milliseconds;

class LIBVALLEY_BASE_EXPORT Workset
{
public:
    using Task = std::function<void()>;

    explicit Workset(std::initializer_list<Task> tasks);
    ~Workset();

    Workset(const Workset&) = delete;
    Workset& operator=(const Workset&) = delete;

    size_t max_task() const;

    void schedule(size_t task_index);

    int poll(size_t max_count = std::numeric_limits<size_t>::max());
    int poll_wait(const Milliseconds& timeout = Milliseconds::max(), size_t max_count = std::numeric_limits<size_t>::max());

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}