// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "ipc/channel.h"

#include <cstdint>
#include <cstdio>

int main()
{
    ipc::Channel channel{ipc::Channel::Config{8, 1}};

    auto subscriber = channel.subscribe();
    auto reader = subscriber->make_reader();

    if (!channel.publish(std::uint64_t{42}))
        return 1;

    if (reader->try_read() && reader->value() == 42)
        std::puts("received 42");
    reader->unpin();

    return 0;
}
