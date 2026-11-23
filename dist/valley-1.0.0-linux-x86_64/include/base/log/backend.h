// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstddef>
#include <string>

#include "config.h"
#include "sink.h"

namespace base {

enum class Async_overflow
{
    BLOCK,
    DROP_OLDEST,
    DROP_NEWEST
};

struct Async_options
{
    std::size_t capacity = 4096;
    Async_overflow overflow = Async_overflow::BLOCK;
};

[[nodiscard]] Sink_ptr make_file_sink(std::string path, bool append = true);
[[nodiscard]] Sink_ptr make_rotating_file_sink(Log_config config);
[[nodiscard]] Sink_ptr make_async_sink(Sink_ptr inner, Async_options options = {});

}  // namespace base
