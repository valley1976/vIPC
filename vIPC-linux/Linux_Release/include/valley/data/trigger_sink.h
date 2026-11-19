#pragma once

#include <memory>

#include "valley/data/export.h"

namespace valley {
namespace data {

// 预留的触发接收端；当前不依赖 serve，保留 pimpl 以便后续接入。
class LIBVALLEY_DATA_EXPORT Trigger_sink
{
public:
    Trigger_sink();
    ~Trigger_sink();

    Trigger_sink(const Trigger_sink&) = delete;
    Trigger_sink& operator=(const Trigger_sink&) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace data
}  // namespace valley
