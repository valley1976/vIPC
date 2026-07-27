#pragma once

#include "valley/conf/export.h"

#include "config.h"

namespace valley {
namespace conf {

struct LIBVALLEY_CONF_EXPORT Configure
{
    static void initialize(const std::string& config);
    static const Config& get_config();
};

}
}