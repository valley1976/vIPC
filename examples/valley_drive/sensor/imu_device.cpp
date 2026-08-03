#include "imu_device.h"

#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdlib>

#include <flatbuffers/flatbuffers.h>

#include "Imu_generated.h"
#include "Time_generated.h"
#include "Quaternion_generated.h"
#include "Vector3_generated.h"

#include "valley/base/thread/thread.h"

using namespace valley;

namespace sensor_sim {

ImuDevice::ImuDevice(const std::string& name, const std::string& channel_name)
    : name_(name), channel_name_(channel_name) {}

ImuDevice::~ImuDevice() {
    stop();
}

void ImuDevice::set_data_callback(DataCallback cb) {
    callback_ = std::move(cb);
}

bool ImuDevice::start(size_t fps) {
    if (!stop_.load()) return false;

    stop_ = false;

    base::Thread_option option;
    option.set_priority_realtime();

    auto start_timepoint = std::chrono::high_resolution_clock::now();

    thread_ = base::start_cyclic(
        option,
        stop_,
        start_timepoint,
        std::chrono::milliseconds(1000 / fps), // 
        [this]()
        {
            generate_sample();
        });

    return true;
}

void ImuDevice::stop() {
    stop_.store(true);
    if (thread_.joinable()) thread_.join();
}

bool ImuDevice::is_running() const {
    return !stop_.load();
}

void ImuDevice::generate_sample() {
    ImuData data;
    data.timestamp = Timestamp::now();
    std::strncpy(data.frame_id, channel_name_.c_str(), sizeof(data.frame_id) - 1);

    time_ += 0.01f;

    data.angular_velocity[0] = 0.1 * std::sin(time_) + (std::rand() % 100 - 50) / 5000.0;
    data.angular_velocity[1] = 0.05 * std::cos(time_ * 0.7) + (std::rand() % 100 - 50) / 5000.0;
    data.angular_velocity[2] = 0.02 * std::sin(time_ * 1.3) + (std::rand() % 100 - 50) / 5000.0;

    data.linear_acceleration[0] = 0.5 * std::sin(time_ * 0.5) + (std::rand() % 100 - 50) / 2000.0;
    data.linear_acceleration[1] = 0.3 * std::cos(time_ * 0.3) + (std::rand() % 100 - 50) / 2000.0;
    data.linear_acceleration[2] = 9.81 + 0.2 * std::sin(time_ * 0.8) + (std::rand() % 100 - 50) / 2000.0;

    double half_angle = time_ * 0.1;
    data.orientation[0] = 0;
    data.orientation[1] = 0;
    data.orientation[2] = std::sin(half_angle);
    data.orientation[3] = std::cos(half_angle);

    sequence_++;

    flatbuffers::FlatBufferBuilder fbb;
    
    auto frame_id = fbb.CreateString(channel_name_);
    
    foxglove::Time timestamp(data.timestamp.sec, data.timestamp.nsec);
    
    auto orientation = foxglove::CreateQuaternion(fbb, 
                                                   data.orientation[0], 
                                                   data.orientation[1], 
                                                   data.orientation[2], 
                                                   data.orientation[3]);
    auto angular_velocity = foxglove::CreateVector3(fbb, 
                                                     data.angular_velocity[0], 
                                                     data.angular_velocity[1], 
                                                     data.angular_velocity[2]);
    auto linear_acceleration = foxglove::CreateVector3(fbb, 
                                                        data.linear_acceleration[0], 
                                                        data.linear_acceleration[1], 
                                                        data.linear_acceleration[2]);
    
    foxglove::ImuBuilder imu_builder(fbb);
    imu_builder.add_timestamp(&timestamp);
    imu_builder.add_frame_id(frame_id);
    imu_builder.add_orientation(orientation);
    imu_builder.add_angular_velocity(angular_velocity);
    imu_builder.add_linear_acceleration(linear_acceleration);
    auto imu = imu_builder.Finish();
    
    fbb.Finish(imu);
    
    if (publisher_) {
        size_t payload_size = fbb.GetSize();
        size_t total_size = sizeof(SensorMessageHeader) + payload_size;
        if (total_size <= publisher_.max_data_size()) {
            void* buf = publisher_.get(total_size);
            if (buf) {
                SensorMessageHeader header;
                header.type = SensorType::kImu;
                header.payload_size = static_cast<uint32_t>(payload_size);
                header.sequence = sequence_;
                header.timestamp = data.timestamp;
                std::strncpy(header.device_name, name_.c_str(), sizeof(header.device_name) - 1);
                std::memcpy(buf, &header, sizeof(header));
                std::memcpy(static_cast<char*>(buf) + sizeof(header), fbb.GetBufferPointer(), payload_size);
                publisher_.write();
                nty_.emit();
            }
        }
    }

    if (callback_) {
        callback_(data, sequence_);
    }
}

bool ImuDevice::publish_to_vipc(const std::string& channel) {
    channel_ = Channel::create(channel);
    if (!channel_)
        return false;

    auto publisher_ = Channel::Publisher::create(channel_);
    if (!publisher_)
        return false;

    nty_ = ipc::Notification("R0", channel);

    return true;
}

} // namespace sensor_sim
