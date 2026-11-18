#pragma once

// supervisor 的 JSON 拓扑配置：
//
// {
//   "application": "valley-xxx",
//   "channel": [{
//     "domain": "/control",
//     "subscriber": ["monitor"],
//     "description": "...",
//     "topic": [{"name": "/control/actuator_command",
//                "subscriber": ["vehicle_can_bridge"],
//                "description": "..."}]
//   }]
// }
//
// 每个 domain 对应一个控制端点（UDS / named pipe），每个 topic 对应一个 Channel。

#include <cstddef>
#include <string>
#include <vector>

namespace exec::supervisor {

struct Topic_config
{
    std::string name;
    std::vector<std::string> subscribers;
    std::string description;
    std::size_t ring_size = 1024;       // topic 级 ring 大小
    // 由 domain/subscriber + topic/subscriber 的去重数量计算；
    // legacy 单 Channel 模式可直接填写。
    std::size_t subscriber_size = 0;
    std::size_t payload_size = 0;       // 0 = 继承 domain
};

struct Domain_config
{
    std::string domain;
    std::vector<std::string> subscribers;   // domain 默认订阅者
    std::string description;
    std::size_t payload_size = 0;       // 0 = scalar
    std::vector<Topic_config> topics;
};

struct Channel_config
{
    // 全局应用名，用于共享内存全局命名。
    std::string application;
    std::vector<Domain_config> domains;
};

// 解析 JSON 文件并做结构校验；失败时 error 带字节位置。
bool load_channel_config(const std::string& path, Channel_config& config,
                         std::string& error);

// domain 默认订阅者 + topic 订阅者（去重）
std::vector<std::string> effective_subscribers(const Domain_config& domain,
                                               const Topic_config& topic);

}  // namespace exec::supervisor
