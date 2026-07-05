#pragma once

#include <cstdint>
#include <chrono>
#include <memory>
#include <thread>

#include "level.h"

namespace valley {
namespace log {

struct Entry
{
    enum { kMax_payload = 256 };

    const char* const file;
    const char* const func;
    const int line;
    const Level level;

    std::chrono::system_clock::time_point ts;
    uint64_t tid;
    char payload[kMax_payload];
    size_t payload_size;

    Entry(Level level, int line, const char* file, const char* func);
};

using Entry_ptr = std::unique_ptr<Entry>;

inline Entry::Entry(Level level, int line, const char* file, const char* func) :
    file(file),
    func(func),
    line(line),
    level(level),
    payload_size(0)
{
}

}
}