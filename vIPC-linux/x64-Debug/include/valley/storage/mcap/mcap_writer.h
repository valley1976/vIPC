#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "valley/storage/export.h"

namespace google {
namespace protobuf {
    class Descriptor;
} }

namespace mcap {
    using SchemaId = uint16_t;
    using ChannelId = uint16_t;

    struct McapWriterOptions;
}

namespace valley {

class LIBVALLEY_STORAGE_EXPORT Mcap_writer
{
public:
    Mcap_writer();
    ~Mcap_writer();

    Mcap_writer(const Mcap_writer&) = delete;
    Mcap_writer& operator=(const Mcap_writer&) = delete;

    Mcap_writer(Mcap_writer&& orig) noexcept;
    Mcap_writer& operator=(Mcap_writer&& orig) noexcept;

    bool open(const std::string& file, const std::string& profile ="");
    bool open(const std::string& file, const mcap::McapWriterOptions& options);

    void close();

    mcap::SchemaId create_schema(const std::string& name, const std::string& encoding, const std::string& data);
    mcap::SchemaId create_ros1_schema(const std::string& ros1_package_resource_name, const std::string& msg_def);
    mcap::SchemaId create_ros2_schema(const std::string& ros1_package_resource_name, const std::string& msg_def);
    mcap::SchemaId create_proto_schema(const google::protobuf::Descriptor* d);
    mcap::SchemaId create_flat_schema(const std::string& fully_qualified_name, const std::string& bfbs_file);

    mcap::ChannelId add_channel(const std::string& topic, const std::string& encoding, mcap::SchemaId schema_id);
    mcap::ChannelId add_ros1_channel(const std::string& topic, mcap::SchemaId ros1_schema_id);
    mcap::ChannelId add_ros2_channel(const std::string& topic, mcap::SchemaId ros2_schema_id);
    mcap::ChannelId add_proto_channel(const std::string& topic, mcap::SchemaId proto_schema_id);
    mcap::ChannelId add_flat_channel(const std::string& topic, mcap::SchemaId flat_schema_id);

    bool write(mcap::ChannelId channel_id, const void* ptr, size_t size, uint32_t sequence = 0, uint64_t publish_time_ns = 0);
    uint64_t written_bytes() const;
    std::string status() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}