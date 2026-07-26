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
#include "valley/ipc/ipc.h"
#include "valley/base/base.h"
#include "valley/data/data.h"


using namespace valley;

const std::string kConfig = R"(
    {
        "application": "ValleyDrive",
        "schema": [
            { "name": "foxglove.RawImage", "encoding": "flatbuffer" },
            { "name": "foxglove.Imu", "encoding": "flatbuffer", "data": "" },
            { "name": "foxglove.LaserScan", "encoding": "flatbuffer" }
        ],
        "domain":[
            {
                "name": "sensor",
                "topic":[
                    {"name": "/sensor/camera_front",  "schema": "foxglove.RawImage", "data_size": 921788, "subscriber": ["realtime", "R0"]},
                    {"name": "/sensor/camera_near",   "schema": "foxglove.RawImage", "subscriber": ["realtime", "R0"]},
                    {"name": "/sensor/camera_left",   "schema": "foxglove.RawImage", "subscriber": ["realtime", "R0"]},
                    {"name": "/sensor/camera_right",  "schema": "foxglove.RawImage", "subscriber": ["realtime", "R0"]},
                    {"name": "/sensor/imu",           "schema": "foxglove.Imu", "subscriber": ["realtime", "R0"]},
                    {"name": "/sensor/lidar",         "schema": "foxglove.LaserScan", "subscriber": ["realtime", "R0"]}
                ]
             },
             {
                "name": "algo",
                "topic":[
                    {"name": "/trajectory",    "subscriber": ["trajectory", "collector"]},
                    {"name": "/control",       "subscriber": ["control", "collector"]}
                ]
             }
        ],
        "executor": [
            { "name": "/collector", 
              "notification": ["collect_pose", 
                       "collect_cam", 
                       "collect_lidar", 
                       "collect_traj",
                       "collect_ctrl"]}
        ],
        "trigger": [
            {
                "name": "T0",
                "description": "demo trigger",
                "topic": [
                    "/sensor/camera_front",
                    "/sensor/camera_near",
                    "/sensor/camera_left",
                    "/sensor/camera_right",
                    "/sensor/imu", 
                    "/sensor/lidar"
                ]
            }
        ],
        "recorder": [
            {
                "name": "R0",
                "bundle": [
                    {
                        "name": "B0",
                        "max_file_size": "50M",
                        "compress": "none",
                        "topic":[
                            "/sensor/camera_front",
                            "/sensor/camera_near",
                            "/sensor/camera_left",
                            "/sensor/camera_right",
                            "/sensor/imu", 
                            "/sensor/lidar"
                        ]
                    }
                ]
            }
        ]
    }
    )";

int main() {
    //base::Log_context::init(base::Level::kInfo);
    conf::Configure::initialize(kConfig);
    ipc::Serve::run(ipc::Model::kBoth);
    data::Recorder recorder("R0");
    
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
        if (sequence % 1000000 == 0)
            vINFO_PRT("imu %s count: %d", imu.frame_id, sequence);
    };

    imu.set_data_callback(imu_stat);

    auto lidar_stat = [](const sensor_sim::LaserScan& lidar, const std::vector<double>&, const std::vector<double>&, uint32_t sequence) {
        if (sequence % 100000 == 0)
            vINFO_PRT("lidar %s count: %d", lidar.frame_id, sequence);
    };

    lidar.set_data_callback(lidar_stat);
    
    camera_front.publish_to_vipc("/sensor/camera_front");
    camera_near.publish_to_vipc("/sensor/camera_near");
    camera_left.publish_to_vipc("/sensor/camera_left");
    camera_right.publish_to_vipc("/sensor/camera_right");

    camera_front.start();
    camera_near.start();
    camera_left.start();
    camera_right.start();

    imu.publish_to_vipc("/sensor/imu");
    imu.start();

    lidar.publish_to_vipc("/sensor/lidar");
    lidar.start();

    std::this_thread::sleep_for(std::chrono::seconds(5));
    recorder.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    recorder.record(data::Trigger::Ptr(new data::Trigger("T0", "demo", 3, 5)));

    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "所有线程已安全退出" << std::endl;

    recorder.stop();

    return 0;
}