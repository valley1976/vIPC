// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>


#include "common/sensor_types.h"

namespace sensor_sim {


class ImuDevice {
public:
    using DataCallback = std::function<void(const ImuData&, uint32_t)>;

    ImuDevice(const std::string& name, const std::string& channel_name);
    ~ImuDevice();

    void set_data_callback(DataCallback cb);

    bool start(size_t fps = 100);
    void stop();
    bool is_running() const;

private:
    void generate_sample();

private:
    std::string name_;
    std::string channel_name_;
    std::atomic<bool> stop_{true};
    std::thread thread_;
    DataCallback callback_;
    uint32_t sequence_ = 0;
    float time_ = 0.0f;
};

} // namespace sensor_sim
