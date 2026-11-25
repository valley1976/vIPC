// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

// supervisor 的 JSONC 拓扑配置（支持 // 与 /* ... */ 注释）：
//
// {
//   "application": "valley-xxx",
//   "channel": [{
//     "group": "/control",
//     "subscriber": ["monitor"],
//     "description": "...",
//     "topic": [{"name": "/control/actuator_command",
//                "subscriber": ["vehicle_can_bridge"],
//                "description": "..."}]
//   }],
//   "shared": [{"name": "param", "size": "512B",
//               "synchronized": [{"name": "state", "size": 256, "align": 64}]}],
//   "notification": [{"name": "FAULT",
//                     "event": ["安全带未系", "相机被遮挡"]}],
//   "recorder": [{"name": "R0",
//                 "bundle": [{"name": "B0",
//                             "topic": ["/camera/front", "/imu"]}]}]
// }
//
// recorder 段会为每个 bundle 自动生成 notification：
//   <recorder>-<bundle> -> topics + "$local_wake"
//
// 一个 supervisor 对应一个控制端点（UDS / named pipe），group 只是配置分组；每个 topic 对应一个 Channel。

#include <cstddef>
#include <string>
#include <vector>

namespace exec::supervisor {

constexpr std::size_t MAX_NOTIFICATION_EVENTS = 256;
constexpr std::size_t MAX_NOTIFICATION_EVENT_LENGTH = 64;

struct Topic_config
{
    std::string name;
    std::vector<std::string> subscribers;
    std::string description;
    std::size_t ring_size = 1024;       // topic 级 ring 大小
    // 由 group/subscriber + topic/subscriber 的去重数量计算；
    // 显式配置 subscriber 容量；默认由订阅者列表推导。
    std::size_t subscriber_size = 0;
    std::size_t payload_size = 0;       // 0 = 继承 group
};

struct Group_config
{
    std::string name;
    std::vector<std::string> subscribers;   // group 默认订阅者
    std::string description;
    std::size_t payload_size = 0;       // 0 = scalar
    std::vector<Topic_config> topics;
};

struct Synchronized_config
{
    std::string name;
    std::size_t size = 0;
    std::size_t alignment = 64;
};

struct Shared_config
{
    std::string name;
    std::size_t size = 0;
    // 可选：由 supervisor 初始化 Synchronized pool。
    std::vector<Synchronized_config> synchronized;
};

// 命名通知：event 下标与 Notification 的事件位一一对应。
struct Notification_config
{
    std::string name;
    std::vector<std::string> events;
};

struct Recorder_bundle_config
{
    std::string name;
    std::vector<std::string> topics;
};

struct Recorder_config
{
    std::string name;
    std::vector<Recorder_bundle_config> bundles;
};

struct Channel_config
{
    // 全局应用名，用于共享内存全局命名。
    std::string application;
    std::vector<Group_config> groups;
    // 预分配的通用共享区域，供 ipc::Synchronized<T> 使用。
    std::vector<Shared_config> shared;
    std::vector<Notification_config> notifications;
    // 可选：supervisor 为每个 bundle 生成 wakeup notification。
    std::vector<Recorder_config> recorders;
};

// 解析 JSON 文件并做结构校验；失败时 error 带字节位置。
bool load_channel_config(const std::string& path, Channel_config& config,
                         std::string& error);

// group 默认订阅者 + topic 订阅者（去重）
std::vector<std::string> effective_subscribers(const Group_config& group,
                                               const Topic_config& topic);

}  // namespace exec::supervisor
