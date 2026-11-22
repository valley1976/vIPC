// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "ipc/event.h"
#include "ipc/notification.h"

#include <chrono>
#include <cstddef>
#include <cstdio>

int main()
{
    alignas(64) std::byte storage[256]{};

    auto notification =
        ipc::Notification::create(storage, sizeof storage, "FAULT");
    if (!notification.valid())
        return 1;
    if (!ipc::Event::bind("FAULT", storage, sizeof storage, {"camera"}))
        return 2;

    ipc::Event event{"FAULT", "camera"};
    event.emit();

    if (notification.wait(0, std::chrono::milliseconds{100}))
        std::puts("received camera event");

    ipc::Event::unbind("FAULT");
    return 0;
}
