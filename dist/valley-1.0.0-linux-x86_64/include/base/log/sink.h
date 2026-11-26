// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <memory>
#include <vector>

#include "base/export.h"
#include "record.h"

namespace base {

class LIBVALLEY_BASE_EXPORT Sink
{
public:
    virtual ~Sink() = default;

    // 每个 sink 自行负责线程安全；同一条记录可能被多个 sink 共享。
    virtual void write(const Log_record& record) = 0;
    virtual void flush() = 0;
};

using Sink_ptr = std::shared_ptr<Sink>;
using Sink_group = std::vector<Sink_ptr>;

LIBVALLEY_BASE_EXPORT Sink_ptr make_std_sink();

}  // namespace base
