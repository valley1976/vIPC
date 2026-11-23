// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "lidar_device.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <utility>

namespace sensor_sim {

LidarDevice::LidarDevice(const std::string& name,
                         const std::string& channel_name)
    : name_(name)
    , channel_name_(channel_name)
{
}

LidarDevice::~LidarDevice()
{
    stop();
}

void LidarDevice::set_scan_params(double start_angle, double end_angle,
                                  uint32_t num_points)
{
    start_angle_ = start_angle;
    end_angle_ = end_angle;
    num_points_ = num_points;
}

void LidarDevice::set_scan_rate(double hz)
{
    if (hz > 0.0)
        scan_rate_hz_ = hz;
}

void LidarDevice::set_data_callback(DataCallback cb)
{
    callback_ = std::move(cb);
}

bool LidarDevice::start(size_t fps)
{
    if (fps == 0 || stop_.load() == false)
        return false;
    stop_.store(false);
    thread_ = std::thread{[this, fps] {
        const auto period = std::chrono::microseconds(1000000 / fps);
        while (!stop_.load()) {
            generate_scan();
            std::this_thread::sleep_for(period);
        }
    }};
    return true;
}

void LidarDevice::stop()
{
    stop_.store(true);
    if (thread_.joinable())
        thread_.join();
}

bool LidarDevice::is_running() const
{
    return !stop_.load();
}

void LidarDevice::generate_scan()
{
    LaserScan scan;
    scan.timestamp = Timestamp::now();
    std::snprintf(scan.frame_id, sizeof scan.frame_id, "%s",
                  channel_name_.c_str());
    scan.start_angle = start_angle_;
    scan.end_angle = end_angle_;
    scan.range_count = num_points_;

    std::vector<double> ranges(num_points_);
    std::vector<double> intensities(num_points_);
    for (uint32_t i = 0; i < num_points_; ++i) {
        const double t = static_cast<double>(i) / num_points_;
        ranges[i] = 10.0 + std::sin(t * 6.28 + time_phase_) * 2.0;
        intensities[i] = 0.5 + std::cos(t * 6.28 + time_phase_) * 0.1;
    }

    if (callback_)
        callback_(scan, ranges, intensities, sequence_);
    ++sequence_;
    time_phase_ += 0.05f;
}

}  // namespace sensor_sim
