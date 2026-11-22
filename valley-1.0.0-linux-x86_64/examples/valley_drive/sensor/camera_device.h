// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <vector>


#include "common/sensor_types.h"

namespace sensor_sim {



class CameraDevice {
public:
    using DataCallback = std::function<void(const CameraImage&, const std::vector<uint8_t>&, uint32_t)>;

    CameraDevice(const std::string& name, const std::string& channel_name);
    ~CameraDevice();

    void set_image_size(uint32_t w, uint32_t h);
    void set_encoding(const std::string& enc);
    void set_data_callback(DataCallback cb);

    bool start(size_t fps = 30);
    void stop();
    bool is_running() const;

private:
    void generate_frame();

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
};

} // namespace sensor_sim
