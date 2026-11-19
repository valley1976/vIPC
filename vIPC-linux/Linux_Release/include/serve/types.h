#pragma once

// 公开的回调类型别名：门面（serve::Server / serve::Client）只暴露这几个，
// 内部 rpc 层用同一组类型，所以实现细节头文件可以留在 src/ 下。

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "serve/message.h"
#include "serve/session_base.h"
#include "serve/status.h"

namespace serve {

// 方法/通知处理器：request 阶段可直接读 message，response 由调用方回填
using Handler = std::function<void(const Session_base::Ptr& session, Message& message,
                                   std::vector<std::byte>& response)>;

// 请求完成回调：status 见 rpc::Status
using Response_handler =
    std::function<void(rpc::Status status, std::vector<std::byte> payload)>;

// 广播请求的完成回调：带上是哪个会话
using Broadcast_handler =
    std::function<void(std::uint64_t session_id, rpc::Status status,
                       std::vector<std::byte> payload)>;

}  // namespace serve
