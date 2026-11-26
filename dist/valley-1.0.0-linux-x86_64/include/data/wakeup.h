// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// Recorder/Bundle 与 supervisor 共享的 wakeup 命名约定。

#include <string>

namespace data {

inline constexpr char LOCAL_WAKE_EVENT[] = "$local_wake";

[[nodiscard]] inline std::string notification_name(
    const std::string& recorder, const std::string& bundle)
{
    return recorder + "-" + bundle;
}

}  // namespace data
