// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "camera_device.h"

#include <chrono>
#include <cstring>
#include <utility>

namespace sensor_sim {

CameraDevice::CameraDevice(const std::string& name,
                           const std::string& channel_name)
    : name_(name)
    , channel_name_(channel_name)
{
}

CameraDevice::~CameraDevice()
{
    stop();
}

void CameraDevice::set_image_size(uint32_t w, uint32_t h)
{
    width_ = w;
    height_ = h;
}

void CameraDevice::set_encoding(const std::string& enc)
{
    encoding_ = enc;
}

void CameraDevice::set_data_callback(DataCallback cb)
{
    callback_ = std::move(cb);
}

bool CameraDevice::start(size_t fps)
{
    if (fps == 0 || stop_.load() == false)
        return false;
    stop_.store(false);
    thread_ = std::thread{[this, fps] {
        const auto period = std::chrono::microseconds(1000000 / fps);
        while (!stop_.load()) {
            generate_frame();
            std::this_thread::sleep_for(period);
        }
    }};
    return true;
}

void CameraDevice::stop()
{
    stop_.store(true);
    if (thread_.joinable())
        thread_.join();
}

bool CameraDevice::is_running() const
{
    return !stop_.load();
}

void CameraDevice::generate_frame()
{
    CameraImage image;
    image.timestamp = Timestamp::now();
    std::snprintf(image.frame_id, sizeof image.frame_id, "%s",
                  channel_name_.c_str());
    image.width = width_;
    image.height = height_;
    std::snprintf(image.encoding, sizeof image.encoding, "%s",
                  encoding_.c_str());
    image.step = width_ * (encoding_ == "rgb8" ? 3 : 1);

    std::vector<uint8_t> pixels(
        static_cast<std::size_t>(image.step) * image.height);
    for (std::size_t i = 0; i < pixels.size(); ++i)
        pixels[i] = static_cast<uint8_t>((sequence_ + i) & 0xFF);

    if (callback_)
        callback_(image, pixels, sequence_);
    ++sequence_;
}

}  // namespace sensor_sim
