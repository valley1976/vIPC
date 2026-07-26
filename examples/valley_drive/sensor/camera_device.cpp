#include "camera_device.h"

#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>

#include "RawImage_generated.h"
#include "Time_generated.h"

#include "valley/base/thread/thread.h"
#include "valley/base/log/log.h"

using namespace valley;

namespace sensor_sim {

CameraDevice::CameraDevice(const std::string& name, const std::string& channel_name)
    : name_(name), channel_name_(channel_name) {}

CameraDevice::~CameraDevice() {
    stop();
}

void CameraDevice::set_image_size(uint32_t w, uint32_t h) {
    width_ = w;
    height_ = h;
}

void CameraDevice::set_encoding(const std::string& enc) {
    encoding_ = enc;
}

void CameraDevice::set_data_callback(DataCallback cb) {
    callback_ = std::move(cb);
}

bool CameraDevice::start(size_t fps) {
    if (!stop_.load()) return false;

    vINFO_PRT("%s start ...", name_.c_str());

    stop_ = false;

    auto interval = 1000/fps;

    base::Thread_option option;
    option.set_priority_realtime();

    auto start_timepoint = base::now();

    thread_ = base::start_cyclic(
        option,
        stop_,
        start_timepoint,
        std::chrono::milliseconds(interval), // 
        [this]()
        {
            generate_frame();
        });

    return true;
}

void CameraDevice::stop() {
    stop_.store(true);
    if (thread_.joinable()) thread_.join();
}

bool CameraDevice::is_running() const {
    return !stop_.load();
}

std::vector<uint8_t> CameraDevice::generate_test_pattern() {
    size_t pixel_size = (encoding_ == "rgb8") ? 3 : 1;
    size_t data_size = width_ * height_ * pixel_size;
    std::vector<uint8_t> data(data_size);

    phase_ += 0.05f;

    for (uint32_t y = 0; y < height_; ++y) {
        for (uint32_t x = 0; x < width_; ++x) {
            size_t idx = (y * width_ + x) * pixel_size;
            float fx = static_cast<float>(x) / width_;
            float fy = static_cast<float>(y) / height_;

            if (encoding_ == "rgb8") {
                data[idx + 0] = static_cast<uint8_t>(255 * (0.5f + 0.5f * std::sin(fx * 6.28f + phase_)));
                data[idx + 1] = static_cast<uint8_t>(255 * (0.5f + 0.5f * std::sin(fy * 6.28f + phase_ * 1.3f)));
                data[idx + 2] = static_cast<uint8_t>(255 * (0.5f + 0.5f * std::sin((fx + fy) * 3.14f + phase_ * 0.7f)));
            } else {
                data[idx] = static_cast<uint8_t>(255 * fx);
            }
        }
    }
    return data;
}

void CameraDevice::generate_frame() {
    auto data = generate_test_pattern();
    sequence_++;

    auto tp = std::chrono::system_clock::now();
    auto s = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp - s);

    uint32_t sec_val = static_cast<uint32_t>(s.time_since_epoch().count());
    uint32_t nsec_val = static_cast<uint32_t>(ns.count());

    flatbuffers::FlatBufferBuilder fbb;

    auto frame_id = fbb.CreateString(channel_name_);
    auto encoding = fbb.CreateString(encoding_);
    auto img_data = fbb.CreateVector(data);

    foxglove::RawImageBuilder img_builder(fbb);
    foxglove::Time timestamp(sec_val, nsec_val);
    img_builder.add_timestamp(&timestamp);
    img_builder.add_frame_id(frame_id);
    img_builder.add_width(width_);
    img_builder.add_height(height_);
    img_builder.add_encoding(encoding);
    img_builder.add_step(width_ * ((encoding_ == "rgb8") ? 3 : 1));
    img_builder.add_data(img_data);
    auto img = img_builder.Finish();

    fbb.Finish(img);

    if (publisher_.is_valid()) {
        size_t payload_size = fbb.GetSize();
        size_t total_size = sizeof(SensorMessageHeader) + payload_size;
        if (total_size <= publisher_.max_data_size()) {
            void* buf = publisher_.get(total_size);
            if (buf) {
                SensorMessageHeader header;
                header.type = SensorType::kCamera;
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
        CameraImage img_info;
        img_info.timestamp.sec = sec_val;
        img_info.timestamp.nsec = nsec_val;
        std::strncpy(img_info.frame_id, channel_name_.c_str(), sizeof(img_info.frame_id) - 1);
        img_info.width = width_;
        img_info.height = height_;
        std::strncpy(img_info.encoding, encoding_.c_str(), sizeof(img_info.encoding) - 1);
        img_info.step = width_ * ((encoding_ == "rgb8") ? 3 : 1);
        callback_(img_info, data, sequence_);
    }
}

bool CameraDevice::publish_to_vipc(const std::string& channel) {
    channel_ = Channel(channel);
    if (!channel_.is_valid()) return false;
    publisher_ = Channel::Publisher(channel_);
    nty_ = Notification("R0", channel);
    return publisher_.is_valid();
}

} // namespace sensor_sim
