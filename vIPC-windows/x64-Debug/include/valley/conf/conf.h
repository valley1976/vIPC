#pragma once

#include "config.h"

namespace valley {
namespace conf {

enum class Model {
    kBroker,
    kSupervisor,
    kBoth
};

void initialize(const std::string& application, Model model = Model::kBroker);

const Config& get_config();

//const Schema* find_topic_schema(const std::string& topic_name);
//int find_topic_id(const std::string& topic);
//const std::vector<size_t>* find_tigger_topic(const std::string& trigger);

}
}