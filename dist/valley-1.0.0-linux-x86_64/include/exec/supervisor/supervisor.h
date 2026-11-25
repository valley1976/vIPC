// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>

#include "exec/supervisor/config.h"

namespace exec {
namespace supervisor {

struct Supervisor_stats
{
    struct Topic
    {
        std::string name;
        std::uint64_t ring_size = 0;
        std::uint64_t subscriber_size = 0;
        std::uint64_t payload_size = 0;
        std::uint64_t published = 0;
        std::uint64_t committed = 0;
        std::uint64_t oldest_position = 0;
        std::uint64_t last_publish_time_ns = 0;
        std::uint64_t publisher_attached = 0;
        std::uint64_t subscriber_attached = 0;
        std::uint64_t subscriber_lag = 0;
        std::uint64_t subscriber_dropped = 0;
        std::uint64_t subscriber_reporting = 0;
    };

    std::uint64_t pid = 0;
    std::uint64_t uptime_ms = 0;
    std::uint64_t shm_bytes = 0;
    std::uint64_t session_count = 0;
    std::uint64_t attach_total = 0;
    std::uint64_t attach_fail = 0;
    std::uint64_t recovery_total = 0;
    std::string metrics_shm;
    std::string metrics_region;
    std::vector<Topic> topics;
};

struct Options
{
    std::string config_path;
    std::string socket_dir = "/tmp";
    bool monitor = false;
#if defined(_WIN32)
    bool service = false;
    std::string service_name;
    std::string pipe_prefix = "\\\\.\\pipe\\vIPC-CHN-";
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

    [[nodiscard]] Supervisor_stats stats() const;
    [[nodiscard]] std::string address() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace supervisor
}  // namespace exec
