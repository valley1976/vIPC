#pragma once

#include <string>

namespace valley {
namespace conf {

enum class Model {
    kBroker,
    kSupervisor,
    kBoth
};

void initialize(const std::string& application, Model model = Model::kBroker);

}
}