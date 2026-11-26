// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "ipc/export.h"

// Channel 的高级接口：恢复流程和 subscriber 观测。
// 普通数据面只需要 ipc/channel.h。

#include <cstddef>
#include <cstdint>
#include <optional>

#include "ipc/channel.h"

namespace ipc {

class Channel_advanced
{
public:
    explicit Channel_advanced(Channel& channel) noexcept : channel_{channel} {}

    [[nodiscard]] bool begin_recovery(std::uint64_t token) noexcept
    {
        return channel_.begin_recovery(token);
    }

    [[nodiscard]] bool end_recovery(std::uint64_t token) noexcept
    {
        return channel_.end_recovery(token);
    }

    [[nodiscard]] bool recover_subscriber(std::uint64_t owner_token,
                                          std::uint64_t token) noexcept
    {
        return channel_.recover_subscriber(owner_token, token);
    }

    [[nodiscard]] bool recover_publisher(std::uint64_t token,
                                         std::uint64_t new_publisher_token) noexcept
    {
        return channel_.recover_publisher(token, new_publisher_token);
    }

    [[nodiscard]] std::optional<Channel::Subscriber_stats> subscriber_stats(
        std::size_t index) const noexcept
    {
        return channel_.subscriber_stats(index);
    }

    [[nodiscard]] std::optional<Channel::Subscriber_metrics_snapshot>
    subscriber_metrics(std::size_t index) const noexcept
    {
        return channel_.subscriber_metrics(index);
    }

    void update_subscriber_metrics(std::size_t index, std::uint64_t owner_token,
                                   std::uint64_t pid,
                                   std::uint64_t last_seen_ns) noexcept
    {
        channel_.update_subscriber_metrics(index, owner_token, pid, last_seen_ns);
    }

private:
    Channel& channel_;
};

}  // namespace ipc
