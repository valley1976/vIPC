#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <chrono>

namespace sensor_sim {

// Time representation matching foxglove Time schema
struct Timestamp {
    uint32_t sec = 0;   // seconds since epoch
    uint32_t nsec = 0;  // nanoseconds within second

    static Timestamp now() {
        auto tp = std::chrono::system_clock::now();
        auto s = std::chrono::time_point_cast<std::chrono::seconds>(tp);
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp - s);
        Timestamp t;
        t.sec = static_cast<uint32_t>(s.time_since_epoch().count());
        t.nsec = static_cast<uint32_t>(ns.count());
        return t;
    }
};

// =============================================================================
// Camera Image - matching foxglove RawImage / CompressedImage schema
// =============================================================================
struct CameraImage {
    Timestamp timestamp;
    char frame_id[64] = {};
    uint32_t width = 0;
    uint32_t height = 0;
    char encoding[32] = {};  // e.g. "rgb8", "jpeg", "png"
    uint32_t step = 0;       // bytes per row
    // Variable length data follows in serialization
};

// =============================================================================
// Laser Scan - matching foxglove LaserScan schema
// =============================================================================
struct LaserScan {
    Timestamp timestamp;
    char frame_id[64] = {};
    double start_angle = 0.0;   // radians
    double end_angle = 0.0;     // radians
    uint32_t range_count = 0;
    // ranges: double[] follows
    // intensities: double[] follows
};

// =============================================================================
// IMU Data - Inertial Measurement Unit
// =============================================================================
struct ImuData {
    Timestamp timestamp;
    char frame_id[64] = {};
    double orientation[4] = {0, 0, 0, 1};      // quaternion x,y,z,w
    double angular_velocity[3] = {0, 0, 0};     // rad/s
    double linear_acceleration[3] = {0, 0, 0};  // m/s^2
};

// =============================================================================
// GPS Location Fix - matching foxglove LocationFix schema
// =============================================================================
struct LocationFix {
    Timestamp timestamp;
    char frame_id[64] = {};
    double latitude = 0.0;   // degrees
    double longitude = 0.0;  // degrees
    double altitude = 0.0;   // meters
};

// =============================================================================
// Unified sensor message for vIPC transmission
// =============================================================================
enum class SensorType : uint32_t {
    kUnknown = 0,
    kCamera = 1,
    kLidar = 2,
    kImu = 3,
    kGps = 4,
};

// Header for all sensor messages transmitted over vIPC
struct SensorMessageHeader {
    SensorType type = SensorType::kUnknown;
    uint32_t payload_size = 0;
    uint32_t sequence = 0;
    Timestamp timestamp;
    char device_name[64] = {};
};

} // namespace sensor_sim
