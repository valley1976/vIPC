#pragma once

#include <string>
#include <vector>
#include <unordered_map>

static constexpr auto kConfig_demo = R"(
{
    "application": "vIPC",
    "description" : "demo",
    "schema": [
        {
            "name": "foo",
            "encoding": "flatbuffer",
            "data": ""
        }
    ],
    "domain": [
        {
            "name": "/sensor",
            "topic":[
                {"name": "/sensor/lidar/front", "schema": "foo", "subscriber": ["R0", "sub2"]},
                {"name": "/sensor/camera/left", "subscriber": ["sub1", "sub2"]}
            ]
        }
    ],
    "executor": [
        { "name": "sensor_colloctor", "notification": ["on_something_happened", "task2"]},
        { "name": "R", "notification": ["on_something_happened", "task2"]}
    ],
    "trigger": [
        {
            "name": "T0",
            "description": "demo trigger",
            "topic": [
                "/sensor/lidar/front"
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
                        "/sensor/lidar/front"
                    ]
                }
            ]
        }
    ]
}
)";

namespace valley {
namespace conf {

struct Schema {
    std::string name;  // unique
    std::string encoding;
    std::string data;
};

// 通道中的主题订阅信息
struct Topic {
    std::string name;
    std::string schema;
    std::string encoding;
    size_t data_size = 64 * 1024;
    size_t cached_count = 64;
    std::vector<std::string> subscriber;
};

// 通道条目（域 + 主题列表）
struct Domain {
    std::string name;
    std::vector<std::string> subscriber;
    std::vector<Topic> topic;   // JSON 中字段名为 "topic"
};

// 执行器条目（名称 + 通知列表）
struct Executor {
    std::string name;
    std::vector<std::string> notification;
};

// 数据采集触发条件
struct Trigger {
    std::string name;
    std::string description;
    std::vector<std::string> topic;
};

// 数据打包配置
struct Bundle {
    std::string name;
    std::string max_file_size;      // 例如 "50M"
    std::string compress;           // 例如 "none"
    std::vector<std::string> topic;
};

// 录制器
struct Recorder {
    std::string name;
    std::string storage_directory;
    //std::vector<std::string> topic; // 所有bundle的topic
    std::vector<Bundle> bundle;
};

// 根对象
struct Config {
    std::string application;
    std::string description;
    std::vector<Schema> schema;
    std::vector<Domain> domain;
    std::vector<Executor> executor;
    std::vector<Trigger> trigger;
    std::vector<Recorder> recorder;
};

}
}