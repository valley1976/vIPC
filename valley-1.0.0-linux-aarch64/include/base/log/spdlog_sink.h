// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// 可选 spdlog 适配器。只有定义 VALLEY_BASE_WITH_SPDLOG 时才引入依赖。
#if defined(VALLEY_BASE_WITH_SPDLOG)

#include <memory>
#include <stdexcept>
#include <utility>

#include <spdlog/spdlog.h>

#include "sink.h"

namespace base {

class Spdlog_sink final : public Sink
{
public:
    explicit Spdlog_sink(std::shared_ptr<spdlog::logger> logger);

    void write(const Log_record& record) override;
    void flush() override;

private:
    std::shared_ptr<spdlog::logger> logger_;
};

inline Spdlog_sink::Spdlog_sink(std::shared_ptr<spdlog::logger> logger)
    : logger_{std::move(logger)}
{
    if (!logger_)
        throw std::invalid_argument{"Spdlog_sink: null logger"};
}

inline void Spdlog_sink::write(const Log_record& record)
{
    spdlog::source_loc location{
        record.file.c_str(), record.line, record.function.c_str()};
    logger_->log(location, static_cast<spdlog::level::level_enum>(
                               static_cast<int>(record.severity)),
                 record.message);
}

inline void Spdlog_sink::flush()
{
    logger_->flush();
}

}  // namespace base

#endif  // VALLEY_BASE_WITH_SPDLOG
