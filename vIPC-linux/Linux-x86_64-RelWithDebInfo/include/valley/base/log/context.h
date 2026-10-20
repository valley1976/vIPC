#pragma once

#include <mutex>
#include <utility>

#include "valley/base/export.h"

#include "sink.h"

namespace valley {
namespace base {

class LIBVALLEY_BASE_EXPORT Log_context
{
public:
    ~Log_context();

    Log_context(const Log_context&) = delete;
    Log_context& operator=(const Log_context&) = delete;

    static void init(Log_severity severity, Sink_ptr&& sink = nullptr);
    static void init(Log_severity severity, Sink_group& group);
    static bool should_log(Log_severity severity);

    static void send(Entry_ptr&& entry);

private:
    Log_context();

    static Log_context& instance();

private:
    class Impl;
    Impl* impl_;
};

}
}