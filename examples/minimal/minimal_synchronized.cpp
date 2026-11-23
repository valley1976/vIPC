// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "ipc/synchronized.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>

namespace {

struct Counter
{
    std::uint64_t value = 0;
};

using Sync = ipc::Synchronized<Counter>;

}  // namespace

int main()
{
    alignas(Sync::required_alignment())
        std::byte storage[Sync::required_size()]{};

    try {
        Sync sync = Sync::create_at(storage, sizeof storage,
                                    Sync::Options{"minimal_sync", {}},
                                    Counter{});

        sync.apply([](Counter& counter) { counter.value = 42; });
        const auto value =
            sync.apply([](const Counter& counter) { return counter.value; });

        std::printf("synchronized value: %llu\n",
                    static_cast<unsigned long long>(value));
        return value == 42 ? 0 : 1;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "minimal_synchronized: %s\n", error.what());
        return 1;
    }
}
