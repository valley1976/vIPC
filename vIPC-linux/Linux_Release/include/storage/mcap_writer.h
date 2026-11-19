#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "storage/export.h"

namespace google {
namespace protobuf {
class Descriptor;
}
}  // namespace google

namespace mcap {
using SchemaId = std::uint16_t;
using ChannelId = std::uint16_t;

struct McapWriterOptions;
}  // namespace mcap

namespace storage {

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
                            const std::string& profile = "");
    [[nodiscard]] bool open(const std::string& file,
                            const mcap::McapWriterOptions& options);

    void close();

    [[nodiscard]] mcap::SchemaId create_schema(const std::string& name,
                                               const std::string& encoding,
                                               const std::string& data);
    [[nodiscard]] mcap::SchemaId create_ros1_schema(
        const std::string& ros1_package_resource_name,
        const std::string& msg_def);
    [[nodiscard]] mcap::SchemaId create_ros2_schema(
        const std::string& ros2_package_resource_name,
        const std::string& msg_def);
    [[nodiscard]] mcap::SchemaId create_proto_schema(
        const google::protobuf::Descriptor* descriptor);
    [[nodiscard]] mcap::SchemaId create_flat_schema(
        const std::string& fully_qualified_name,
        const std::string& bfbs_file);

    [[nodiscard]] mcap::ChannelId add_channel(const std::string& topic,
                                              const std::string& encoding,
                                              mcap::SchemaId schema_id);
    [[nodiscard]] mcap::ChannelId add_ros1_channel(
        const std::string& topic, mcap::SchemaId ros1_schema_id);
    [[nodiscard]] mcap::ChannelId add_ros2_channel(
        const std::string& topic, mcap::SchemaId ros2_schema_id);
    [[nodiscard]] mcap::ChannelId add_proto_channel(
        const std::string& topic, mcap::SchemaId proto_schema_id);
    [[nodiscard]] mcap::ChannelId add_flat_channel(
        const std::string& topic, mcap::SchemaId flat_schema_id);

    [[nodiscard]] bool write(mcap::ChannelId channel_id, const void* ptr,
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
