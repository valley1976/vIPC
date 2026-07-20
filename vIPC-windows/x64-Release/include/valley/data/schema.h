#pragma once

#include <string>
#include <map>

namespace google {
namespace protobuf {
class Descriptor;
}
}

namespace valley {
namespace data {

struct Schema {
    std::string type_name;  // unique
    std::string encoding;
    std::string data;
};

class Schema_manager {
public:
    ~Schema_manager() = default;
    
    Schema_manager(const Schema_manager&) = delete;
    Schema_manager& operator=(const Schema_manager&) = delete;

    static void initialize_from_configuration();
    static void registry(std::string& type_name, std::string& encoding, std::string& data);
    static void registry(const google::protobuf::Descriptor* d);
    static void registry_flatbuffer(std::string fully_qualified_name, const std::string& fbs_binary_file);
    static const Schema* find(const std::string& type_name);

private:
    Schema_manager() = default;
    static Schema_manager& instance();

private:
    std::map<std::string, Schema> schema_map_;
};

}
}