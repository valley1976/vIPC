// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "config.h"
#include "sink.h"

namespace base {

// 静态门面；实际分发由 Log_system 完成。
class LIBVALLEY_BASE_EXPORT Log_context
{
public:
    static void init(Log_severity severity, Sink_ptr sink = nullptr);
    static void init(Log_severity severity, Sink_group& group);
    static void init_from_config(const std::string& path = {});
    static void set_level(Log_severity severity);
    static bool should_log(Log_severity severity);

    static void send(Log_record record);
    static void flush();
    static void shutdown();
};

}  // namespace base
