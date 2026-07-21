#pragma once

#include <string>
#include <unordered_map>

#include "valley/conf/config.h"

namespace google {
namespace protobuf {
class Descriptor;
}
}

namespace valley {
namespace data {

class Schema_manager {
public:
    ~Schema_manager() = default;
    
    Schema_manager(const Schema_manager&) = delete;
    Schema_manager& operator=(const Schema_manager&) = delete;

    static void registry(std::string& name, std::string& encoding, std::string& data);
    static void registry(const google::protobuf::Descriptor* d);
    static void registry_flatbuffer(std::string fully_qualified_name, const std::string& fbs_binary_file);
    static const conf::Schema* find(const std::string& name);

private:
    Schema_manager() = default;
    static Schema_manager& instance();

private:
    std::unordered_map<std::string, conf::Schema> schema_map_;
};

}
}