// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <string>

#include "storage/export.h"

namespace storage {

using Schema_id = std::uint16_t;
using Channel_id = std::uint16_t;

enum class Compression
{
    NONE,
    ZSTD,
    LZ4
};

struct Writer_options
{
    std::string profile;
    Compression compression = Compression::NONE;
};

// MCAP 写入器：封装 mcap::McapWriter，并负责 schema/channel 注册。
class LIBVALLEY_STORAGE_EXPORT Mcap_writer
{
public:
    Mcap_writer();
    ~Mcap_writer();

    Mcap_writer(const Mcap_writer&) = delete;
    Mcap_writer& operator=(const Mcap_writer&) = delete;

    Mcap_writer(Mcap_writer&& orig) noexcept;
    Mcap_writer& operator=(Mcap_writer&& orig) noexcept;

    [[nodiscard]] bool open(const std::string& file,
                            const Writer_options& options = {});

    void close();

    [[nodiscard]] Schema_id create_schema(const std::string& name,
                                               const std::string& encoding,
                                               const std::string& data);
    [[nodiscard]] Schema_id create_ros1_schema(
        const std::string& ros1_package_resource_name,
        const std::string& msg_def);
    [[nodiscard]] Schema_id create_ros2_schema(
        const std::string& ros2_package_resource_name,
        const std::string& msg_def);
    [[nodiscard]] Schema_id create_flat_schema(
        const std::string& fully_qualified_name,
        const std::string& bfbs_file);

    [[nodiscard]] Channel_id add_channel(const std::string& topic,
                                              const std::string& encoding,
                                              Schema_id schema_id);
    [[nodiscard]] Channel_id add_ros1_channel(
        const std::string& topic, Schema_id ros1_schema_id);
    [[nodiscard]] Channel_id add_ros2_channel(
        const std::string& topic, Schema_id ros2_schema_id);
    [[nodiscard]] Channel_id add_flat_channel(
        const std::string& topic, Schema_id flat_schema_id);

    [[nodiscard]] bool add_metadata(
        const std::string& name,
        const std::unordered_map<std::string, std::string>& metadata);

    [[nodiscard]] bool write(Channel_id channel_id, const void* ptr,
                             std::size_t size, std::uint32_t sequence = 0,
                             std::uint64_t publish_time_ns = 0,
                             std::uint64_t log_time_ns = 0);
    [[nodiscard]] std::uint64_t written_bytes() const;
    [[nodiscard]] std::string status() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace storage
