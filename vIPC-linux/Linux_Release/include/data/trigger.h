#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "data/export.h"
#include "data/types.h"

namespace data {

// 触发事件：描述一次录制窗口及其关联文件。
class LIBVALLEY_DATA_EXPORT Trigger
{
public:
    using Ptr = std::unique_ptr<Trigger>;

    const std::string name;
    const std::string origin;

    const Time_point trigger_time;
    const Time_point start_time;
    const Time_point end_time;

    std::uint64_t sequence = 0;
    // Recorder 级 Trigger 序号；多 Bundle 共用一个 group_sequence。
    std::uint64_t group_sequence = 0;
    std::vector<std::string> files;
    std::string manifest_file;
    std::string status = "COMPLETE";

    std::string description;
    std::vector<std::string> topic;

    Trigger(const std::string& name, const std::string& origin,
            std::chrono::milliseconds trigger_time_point, std::size_t before_sec,
            std::size_t after_sec);

    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;

    [[nodiscard]] std::chrono::milliseconds trigger_time_ms() const;
    [[nodiscard]] std::size_t before_sec() const;
    [[nodiscard]] std::size_t after_sec() const;

    [[nodiscard]] static Ptr make_unique(const std::string& name,
                                         const std::string& origin,
                                         std::chrono::milliseconds trigger_time_point,
                                         std::size_t before_sec,
                                         std::size_t after_sec);

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] static Ptr from_string(const std::string& str);
    [[nodiscard]] Ptr clone() const;
};

}  // namespace data
