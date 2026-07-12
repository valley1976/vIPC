// SPDX-License-Identifier: MIT
// Copyright (c) 2026 gus. All rights reserved.

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <vector>
#include <functional>

#include "sensor/camera_device.h"
#include "sensor/imu_device.h"
#include "sensor/lidar_device.h"

#include "valley/conf/conf.h"
#include "valley/shm/channel.h"
#include "valley/exec/executor.h"
#include "valley/exec/notification.h"
#include "valley/thread/thread.h"
#include "valley/log/log.h"

using namespace valley;

const std::string kConfig = R"(
    {
        "application": "ValleyDrive",
        "channel":[
            {
            "domain": "simu",
            "topic":[
                {"name": "/trajectory",    "subscriber": ["trajectory", "collector"]},
                {"name": "/control",       "subscriber": ["control", "collector"]},
                {"name": "/sensor/camera_front",  "subscriber": ["realtime", "collector"]},
                {"name": "/sensor/camera_near",   "subscriber": ["realtime", "collector"]},
                {"name": "/sensor/camera_left",   "subscriber": ["realtime", "collector"]},
                {"name": "/sensor/camera_right",  "subscriber": ["realtime", "collector"]},
                {"name": "/sensor/imu",           "subscriber": ["realtime", "collector"]},
                {"name": "/sensor/lidar",         "subscriber": ["realtime", "collector"]}
            ]}
        ],
        "executor": [
            { "name": "/collector", 
              "notification": ["collect_pose", 
                       "collect_cam", 
                       "collect_lidar", 
                       "collect_traj",
                       "collect_ctrl"]}
        ]
    }
    )";

int main() {
    log::Context::instance().init(log::Level::kInfo);

    conf::initialize(kConfig, conf::Model::kBoth);
    
    sensor_sim::CameraDevice camera_front("camera front",  "/sensor/camera_front");
    sensor_sim::CameraDevice camera_near("camera near",    "/sensor/camera_near");
    sensor_sim::CameraDevice camera_left("camera left",    "/sensor/camera_left");
    sensor_sim::CameraDevice camera_right("camera right",  "/sensor/camera_right");

    sensor_sim::ImuDevice imu("imu", "/sensor/imu");

    sensor_sim::LidarDevice lidar("lidar", "/sensor/lidar");

    auto came_stat = [](const sensor_sim::CameraImage& came, const std::vector<uint8_t>&, uint32_t sequnce) {
        if (sequnce % 100 == 0)
            vINFO_PRT("came %s count: %d", came.frame_id, sequnce);
    };
    camera_front.set_data_callback(came_stat);
    camera_near.set_data_callback(came_stat);
    camera_left.set_data_callback(came_stat);
    camera_right.set_data_callback(came_stat);

    auto imu_stat = [](const sensor_sim::ImuData& imu, uint32_t sequence) {
        if (sequence % 100 == 0)
            vINFO_PRT("imu %s count: %d", imu.frame_id, sequence);
    };

    imu.set_data_callback(imu_stat);

    auto lidar_stat = [](const sensor_sim::LaserScan& lidar, const std::vector<double>&, const std::vector<double>&, uint32_t sequence) {
        if (sequence % 100 == 0)
            vINFO_PRT("lidar %s count: %d", lidar.frame_id, sequence);
    };

    lidar.set_data_callback(lidar_stat);

    camera_front.start();
    camera_near.start();
    camera_left.start();
    camera_right.start();

    imu.start();
    lidar.start();

    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "所有线程已安全退出" << std::endl;
    return 0;
}