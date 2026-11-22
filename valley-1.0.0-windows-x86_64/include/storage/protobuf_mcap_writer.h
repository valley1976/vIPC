// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "storage/export.h"
#include "storage/mcap_writer.h"
#include "storage/protobuf_export.h"

#include <map>

namespace google {
namespace protobuf {
class Descriptor;
}
}  // namespace google

namespace storage {

// Protobuf schema/channel 辅助库；与基础 Mcap_writer 隔离。
class LIBVALLEY_STORAGE_PROTOBUF_EXPORT Protobuf_mcap_writer
{
public:
    explicit Protobuf_mcap_writer(Mcap_writer& writer);

    [[nodiscard]] Schema_id create_schema(
        const google::protobuf::Descriptor* descriptor);
    [[nodiscard]] Channel_id add_channel(
        const std::string& topic, Schema_id schema_id);

private:
    Mcap_writer* writer_ = nullptr;
    std::map<const google::protobuf::Descriptor*, Schema_id> schema_ids_;
};

}  // namespace storage
