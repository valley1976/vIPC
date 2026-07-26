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


class CameraDevice {
public:
    using DataCallback = std::function<void(const CameraImage&, const std::vector<uint8_t>&, uint32_t)>;

    CameraDevice(const std::string& name, const std::string& channel_name);
    ~CameraDevice();

    bool publish_to_vipc(const std::string& channel);

    void set_image_size(uint32_t w, uint32_t h);
    void set_encoding(const std::string& enc);
    void set_data_callback(DataCallback cb);

    bool start(size_t fps = 30);
    void stop();
    bool is_running() const;

private:
    void generate_frame();
    std::vector<uint8_t> generate_test_pattern();

private:
    std::string name_;
    std::string channel_name_;
    uint32_t width_ = 640;
    uint32_t height_ = 480;
    std::string encoding_ = "rgb8";
    std::atomic<bool> stop_{true};
    std::thread thread_;
    DataCallback callback_;
    uint32_t sequence_ = 0;
    float phase_ = 0.0f;

    Channel channel_;
    Channel::Publisher publisher_;
    Notification nty_;
};

} // namespace sensor_sim
