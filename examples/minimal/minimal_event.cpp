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

    auto notification = ipc::Notification::create(
        storage, sizeof storage, "FAULT", {"camera"});
    if (!notification.valid())
        return 1;
    if (!ipc::Event::bind("FAULT", notification))
        return 2;

    ipc::Event event{"FAULT", "camera"};
    if (!event.valid())
        return 3;

    event.emit();

    const bool received =
        notification.wait(std::chrono::milliseconds{100});
    if (received && notification.poll() == 0)
        std::puts("received camera event");

    ipc::Event::unbind("FAULT");
    return received ? 0 : 4;
}
