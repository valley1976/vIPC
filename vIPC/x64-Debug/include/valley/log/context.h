#pragma once

#include <mutex>
#include <utility>

#include "sink.h"

namespace valley {
namespace log {

class Context
{
public:
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    static Context& instance();

    void init(Level level, Sink_ptr&& sink = nullptr);
    void init(Level level, Sink_group& group);
    bool should_log(Level level) const;

    void send(Entry_ptr&& entry);

private:
    Context();

private:
    class Impl;
    Impl* impl_;
};

}
}