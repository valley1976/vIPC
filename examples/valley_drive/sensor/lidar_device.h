#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <vector>

#include <valley/ipc/ipc.h>

#include "common/sensor_types.h"

namespace sensor_sim {

using valley::ipc::Channel;
using valley::ipc::Notification;

class LidarDevice {
public:
    using DataCallback = std::function<void(const LaserScan&, const std::vector<double>&, const std::vector<double>&, uint32_t)>;

    LidarDevice(const std::string& name, const std::string& channel_name);
    ~LidarDevice();

    void set_scan_params(double start_angle, double end_angle, uint32_t num_points);
    void set_scan_rate(double hz);
    void set_data_callback(DataCallback cb);
    bool publish_to_vipc(const std::string& vipc_channel);

    bool start(size_t fps = 10);
    void stop();
    bool is_running() const;

private:
    void generate_scan();

private:
    std::string name_;
    std::string channel_name_;
    double start_angle_ = -3.14159;
    double end_angle_ = 3.14159;
    uint32_t num_points_ = 360;
    double scan_rate_hz_ = 10.0;
    std::atomic<bool> stop_{true};
    std::thread thread_;
    DataCallback callback_;
    uint32_t sequence_ = 0;
    float time_phase_ = 0.0f;

    Channel channel_;
    Channel::Publisher publisher_;
    Notification nty_;
};

} // namespace sensor_sim
