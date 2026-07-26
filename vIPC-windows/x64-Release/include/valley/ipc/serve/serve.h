#pragma once

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {

enum class Model {
    kBroker,
    kSupervisor,
    kBoth
};

class LIBVALLEY_IPC_EXPORT Serve {
public:
    static void run(Model mode = Model::kBroker);
};

}
}