#pragma once

#include "config.h"

#include "valley/export/export.h"

namespace valley {
namespace conf {

enum class Model {
    kBroker,
    kSupervisor,
    kBoth
};

struct LIBVALLEY_EXPORT Configure
{
    static void initialize(const std::string& application, Model model = Model::kBroker);
    static const Config& get_config();
};

}
}