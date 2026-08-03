#include "lidar_device.h"

#include <cmath>
#include <chrono>
#include <cstring>
#include <iostream>

#include "LaserScan_generated.h"
#include "Time_generated.h"

#include "valley/base/thread/thread.h"

using namespace valley;

namespace sensor_sim {

LidarDevice::LidarDevice(const std::string& name, const std::string& channel_name)
    : name_(name), channel_name_(channel_name) {}

LidarDevice::~LidarDevice() {
    stop();
}

void LidarDevice::set_scan_params(double start_angle, double end_angle, uint32_t num_points) {
    start_angle_ = start_angle;
    end_angle_ = end_angle;
    num_points_ = num_points;
}

void LidarDevice::set_data_callback(DataCallback cb) {
    callback_ = std::move(cb);
}

bool LidarDevice::start(size_t fps) {
    if (!stop_.load()) return false;

    stop_ = false;

    base::Thread_option option;
    option.set_priority_realtime();

    auto start_timepoint = base::now();

    thread_ = base::start_cyclic(
        option,
        stop_,
        start_timepoint,
        std::chrono::milliseconds(1000 / fps), // 
        [this]()
        {
            generate_scan();
        });

    return true;
}

void LidarDevice::stop() {
    stop_.store(true);
    if (thread_.joinable()) thread_.join();
}

bool LidarDevice::is_running() const {
    return !stop_.load();
}

void LidarDevice::generate_scan() {
    std::vector<double> ranges(num_points_);
    std::vector<double> intensities(num_points_);

    time_phase_ += 0.1f;

    for (uint32_t i = 0; i < num_points_; ++i) {
        double angle = start_angle_ + (end_angle_ - start_angle_) * i / num_points_;
        double base_range = 10.0 + 5.0 * std::sin(angle * 3.0f + time_phase_);
        if (std::abs(std::sin(angle * 2.0)) > 0.9) {
            base_range = 3.0;
        }
        ranges[i] = std::max(0.1, base_range + (std::rand() % 100) / 500.0);
        intensities[i] = 100.0 + 50.0 * (1.0 - ranges[i] / 15.0);
    }

    sequence_++;

    auto tp = std::chrono::system_clock::now();
    auto s = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp - s);

    uint32_t sec_val = static_cast<uint32_t>(s.time_since_epoch().count());
    uint32_t nsec_val = static_cast<uint32_t>(ns.count());

    flatbuffers::FlatBufferBuilder fbb;

    auto frame_id = fbb.CreateString(channel_name_);
    auto ranges_vec = fbb.CreateVector(ranges);
    auto intensities_vec = fbb.CreateVector(intensities);

    foxglove::LaserScanBuilder scan_builder(fbb);
    foxglove::Time timestamp(sec_val, nsec_val);
    scan_builder.add_timestamp(&timestamp);
    scan_builder.add_frame_id(frame_id);
    scan_builder.add_start_angle(start_angle_);
    scan_builder.add_end_angle(end_angle_);
    scan_builder.add_ranges(ranges_vec);
    scan_builder.add_intensities(intensities_vec);
    auto scan = scan_builder.Finish();

    fbb.Finish(scan);

    if (publisher_) {
        size_t payload_size = fbb.GetSize();
        size_t total_size = sizeof(SensorMessageHeader) + payload_size;
        if (total_size <= publisher_.max_data_size()) {
            void* buf = publisher_.get(total_size);
            if (buf) {
                SensorMessageHeader header;
                header.type = SensorType::kLidar;
                header.payload_size = static_cast<uint32_t>(payload_size);
                header.sequence = sequence_;
                header.timestamp.sec = sec_val;
                header.timestamp.nsec = nsec_val;
                std::strncpy(header.device_name, name_.c_str(), sizeof(header.device_name) - 1);
                std::memcpy(buf, &header, sizeof(header));
                std::memcpy(static_cast<char*>(buf) + sizeof(header), fbb.GetBufferPointer(), payload_size);
                publisher_.write();
                nty_.emit();
            }
        }
    }

    if (callback_) {
        LaserScan scan_info;
        scan_info.timestamp.sec = sec_val;
        scan_info.timestamp.nsec = nsec_val;
        std::strncpy(scan_info.frame_id, channel_name_.c_str(), sizeof(scan_info.frame_id) - 1);
        scan_info.start_angle = start_angle_;
        scan_info.end_angle = end_angle_;
        scan_info.range_count = num_points_;
        callback_(scan_info, ranges, intensities, sequence_);
    }
}

bool LidarDevice::publish_to_vipc(const std::string& channel) {
    channel_ = Channel::create(channel);
    if(!channel_)
        return false;

    auto publisher_ = Channel::Publisher::create(channel_);
    if(!publisher_)
        return false;

    nty_ = ipc::Notification("R0", channel);
    
    return true;
}

} // namespace sensor_sim
