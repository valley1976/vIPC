#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "data/export.h"

namespace data {

enum class Write_mode
{
    SYNC,
    ASYNC
};

// 一个 bundle 对应一个独立 MCAP writer。
struct Bundle_config
{
    std::string name;
    std::size_t max_file_size = 100 * 1024 * 1024;
    std::chrono::milliseconds max_file_duration{0};
    std::string compression = "none";
    Write_mode write_mode = Write_mode::SYNC;
    std::size_t max_queue_bytes = 64 * 1024 * 1024;
    std::vector<std::string> topics;
};

// 一个 Recorder 可以拥有多个 bundle；topic 在 bundle 之间必须互斥。
struct Recorder_config
{
    std::string name;
    std::string metrics_shm;
    std::vector<Bundle_config> bundles;
};

// 支持 "50M"、"200K"、"1G" 和纯字节数；按 1024 进制解析。
[[nodiscard]] LIBVALLEY_DATA_EXPORT std::optional<std::size_t>
parse_size(const std::string& text);

// 解析 {"recorder":[{"name":...,"bundle":[...]}]}。
// 成功时 configs 非空，并已完成名称/topic 去重校验。
[[nodiscard]] LIBVALLEY_DATA_EXPORT bool
parse_recorder_configs(const std::string& json,
                       std::vector<Recorder_config>& configs,
                       std::string& error);

// 校验多条 Recorder 配置：
// - Recorder name 全局唯一；
// - Bundle name 在同一 Recorder 内唯一；
// - topic 在同一 Bundle 内不重复；
// - topic 在同一 Recorder 的 Bundle 之间不重复。
[[nodiscard]] LIBVALLEY_DATA_EXPORT bool
validate_recorder_configs(const std::vector<Recorder_config>& configs,
                          std::string& error);

}  // namespace data
