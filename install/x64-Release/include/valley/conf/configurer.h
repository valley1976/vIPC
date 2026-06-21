#pragma once

#include <string>

namespace valley {
namespace conf {

struct Configurer
{
    static void start_supervisor(const std::string& config_file);
};

}
}