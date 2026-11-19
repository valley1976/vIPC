#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

#include "exec/supervisor/channel_config.h"
#include "exec/supervisor/supervisor_rpc.h"

namespace exec {
namespace supervisor {

struct Options
{
    std::string config_path;
    std::string socket_dir = "/tmp";
    std::string name = "default";
    std::string socket_path;
    std::size_t ring_size = 1024;
    std::size_t subscriber_size = 63;
    std::size_t payload_size = 0;
    bool has_legacy = false;
    bool monitor = false;
#if defined(_WIN32)
    bool service = false;
    std::string service_name;
    std::string pipe_prefix = "\\\\.\\pipe\\dsh-channel-";
#endif
};

// 数据面控制面：按配置创建 shm + Channel，处理 serve attach。
class Supervisor
{
public:
    Supervisor(Channel_config config, Options options);
    ~Supervisor();

    Supervisor(const Supervisor&) = delete;
    Supervisor& operator=(const Supervisor&) = delete;

    [[nodiscard]] bool start();
    void run();
    void request_stop();

    [[nodiscard]] Stats_response stats() const;
    [[nodiscard]] std::string address() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace supervisor
}  // namespace exec
