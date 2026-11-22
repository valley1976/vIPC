// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "imu_device.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <utility>

namespace sensor_sim {

ImuDevice::ImuDevice(const std::string& name,
                     const std::string& channel_name)
    : name_(name)
    , channel_name_(channel_name)
{
}

ImuDevice::~ImuDevice()
{
    stop();
}

void ImuDevice::set_data_callback(DataCallback cb)
{
    callback_ = std::move(cb);
}

bool ImuDevice::start(size_t fps)
{
    if (fps == 0 || stop_.load() == false)
        return false;
    stop_.store(false);
    thread_ = std::thread{[this, fps] {
        const auto period = std::chrono::microseconds(1000000 / fps);
        while (!stop_.load()) {
            generate_sample();
            std::this_thread::sleep_for(period);
        }
    }};
    return true;
}

void ImuDevice::stop()
{
    stop_.store(true);
    if (thread_.joinable())
        thread_.join();
}

bool ImuDevice::is_running() const
{
    return !stop_.load();
}

void ImuDevice::generate_sample()
{
    ImuData data;
    data.timestamp = Timestamp::now();
    std::snprintf(data.frame_id, sizeof data.frame_id, "%s",
                  channel_name_.c_str());
    data.orientation[0] = 0.0;
    data.orientation[1] = 0.0;
    data.orientation[2] = std::sin(time_) * 0.01;
    data.orientation[3] = 1.0;
    data.angular_velocity[2] = 0.01 + std::sin(time_) * 0.001;
    data.linear_acceleration[0] = std::sin(time_ * 2.0) * 0.1;
    data.linear_acceleration[1] = std::cos(time_ * 2.0) * 0.1;

    if (callback_)
        callback_(data, sequence_);
    ++sequence_;
    time_ += 0.01f;
}

}  // namespace sensor_sim
