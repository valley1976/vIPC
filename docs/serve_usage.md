# serve 使用指南

从零接入 `serve` 的步骤、常见写法与坑。设计见 [serve_design.md](serve_design.md)，
逐个头文件的接口见 [serve_api.md](serve_api.md)。

## 1. 构建

```powershell
cmake -S serve -B build_serve -G "Visual Studio 17 2022" -A x64
cmake --build build_serve --config Release
```

默认用 `D:/libs/asio-1.36.0/include` 的 asio；换机器时加
`-DVALLEY_ASIO_INCLUDE=<asio 的 include 目录>`。

Linux / WSL 下同一套命令，只换生成器和 asio 路径（Linux 走 POSIX 本机套接字分支）：

```bash
cmake -S serve -B build_serve -DCMAKE_BUILD_TYPE=Release \
      -DVALLEY_ASIO_INCLUDE=/mnt/d/libs/asio-1.36.0/include
cmake --build build_serve -j4
ctest --test-dir build_serve --output-on-failure
```

构建产物：

| target | 说明 |
|---|---|
| `serve` | 静态库 |
| `tcp_echo_example` | TCP 服务端 + 客户端一体示例 |
| `ipc_ping_pong_example` | IPC 请求-响应示例 |
| `timer_example` | 定时器示例 |
| `tcp_reconnect_example` | 服务端踢会话后客户端退避重连的完整示例 |
| `event_loop_test` / `tcp_echo_test` / `ipc_test` / `rpc_test` / `xproc_test` / `tcp_frame_test` / `tcp_rpc_client_test` / `tcp_rpc_test` / `address_test` / `facade_test` / `handshake_test` / `limits_test` / `stop_test` / `caps_test` / `connect_test` / `graceful_test` / `fuzz_test` / `stress_test` / `keepalive_test` / `singleflight_test` | 回归测试（`ctest`） |

示例都在 [`valley/serve/examples/`](../valley/serve/examples) 下，直接运行即可：

```powershell
.\build_serve\Release\tcp_echo_example.exe
.\build_serve\Release\ipc_ping_pong_example.exe
.\build_serve\Release\timer_example.exe
.\build_serve\Release\tcp_reconnect_example.exe
```

`ctest -C Release` 会把 20 个回归测试和 4 个示例一起跑一遍（示例各自有 20 s 超时）。

输出形如：

```
listening on 0.0.0.0:50892
echo back: hello
```

## 2. 在自己的工程里接入

```cmake
set(VALLEY_ASIO_INCLUDE "D:/libs/asio-1.36.0/include" CACHE PATH "asio include 目录")
add_subdirectory(serve)                       # 或先单独构建 install
target_link_libraries(my_app PRIVATE serve)
```

`serve` 会把 include 目录、`ASIO_STANDALONE`、`ws2_32/mswsock/advapi32`
作为 `PUBLIC` 属性传出去，使用者不用重复配置。代码里 `#include "serve/serve.h"`
或按需包含单个头文件。

## 3. TCP 服务端

```cpp
#include "serve/event_loop.h"
#include "serve/src/tcp/server.h"

serve::Event_loop loop("app");
loop.start();                                  // 后台线程跑 io_context

auto handler = std::make_shared<serve::Tcp_session::Handler>();
handler->on_connected = [](const serve::Tcp_session::Ptr& session) {
    std::printf("connected id=%llu from %s\n",
                (unsigned long long)session->id(), session->remote_address().c_str());
};
handler->on_received = [](const serve::Tcp_session::Ptr& session, const void* data,
                          std::size_t size, std::size_t& consumed) {
    // 这里按自己的协议拆包；consume 多少由你决定，剩下的留在缓冲里
    (void)session->send(data, size);
    consumed = size;
};
handler->on_error = [](const serve::Tcp_session::Ptr&, const std::error_code& ec) {
    std::fprintf(stderr, "session error: %s\n", ec.message().c_str());
};

auto server = std::make_shared<serve::Tcp_server>(loop, 0, true);   // 0 = 系统分配端口
server->set_handler(handler);
if (!server->start()) {
    std::fprintf(stderr, "start failed: %s\n", server->address_error().message().c_str());
    return 1;
}
std::printf("listening on %d\n", server->port());
```

要点：

- `port()` 在 `start()` 之后才准确（传 0 时由系统分配）；
- `broadcast()` / `disconnect_all()` / `find_session()` 都在 loop 线程调用；
- `connected_sessions()` / `find_session()` 可以从别的线程查（内部有 `shared_mutex`）。

完整可运行版本见 [tcp_echo.cpp](../valley/serve/examples/tcp_echo.cpp)。

## 4. TCP 客户端：连接、超时、重连

```cpp
auto client = std::make_shared<serve::Tcp_client>(loop, "127.0.0.1", 9000);

auto handler = std::make_shared<serve::Tcp_session::Handler>();
handler->on_connected = [](const serve::Tcp_session::Ptr& session) {
    (void)session->send("hello", 5);
};
handler->on_received = [](const serve::Tcp_session::Ptr&, const void* data, std::size_t size,
                          std::size_t& consumed) {
    // ...
    consumed = size;
};
client->set_handler(handler);
client->set_error_handler([](const std::error_code& ec) {
    std::fprintf(stderr, "connect failed: %s\n", ec.message().c_str());
});
client->connect_async();
```

**连接超时**用定时器兜底（`on_connected` 里取消即可）：

```cpp
serve::Steady_timer timeout(loop);
timeout.expire_after([client](bool expired) {
    if (expired && !client->is_connected()) {
        client->disconnect();
        std::fprintf(stderr, "connect timeout\n");
    }
}, std::chrono::seconds(3));
```

**重连**：断开后由业务决定何时重试，建议加退避，别在回调里立刻重连：

```cpp
handler->on_disconnected = [&loop, client](const serve::Tcp_session::Ptr&) {
    // 这里已经在 loop 线程；用定时器做 1s 退避
    static serve::Steady_timer backoff(loop);   // 实际工程里放在对象成员里
    backoff.expire_after([client](bool expired) {
        if (expired)
            client->reconnect_async();
    }, std::chrono::seconds(1));
};
```

`Tcp_client` 自身提供 `reconnect_async()`（置标志、断开后自动重连），
退避节奏由调用方控制；也可以像上面这样在 `on_disconnected` 里自己排定时器。
完整可运行版本见 [tcp_reconnect.cpp](../valley/serve/examples/tcp_reconnect.cpp)（服务端踢掉会话 → 200 ms 退避 → 重连后继续回声）。

## 5. IPC 请求-响应

```cpp
const std::string path =
#if defined(_WIN32)
    "\\\\.\\pipe\\my_service";
#else
    "/tmp/my_service.sock";
#endif

auto server = std::make_shared<serve::Ipc_server>(loop, path);
auto server_handler = std::make_shared<serve::Ipc_session::Handler>();
server_handler->on_message = [](const serve::Ipc_session::Ptr& session,
                                serve::Message& message) {
    if (message.kind() == serve::Kind::REQUEST) {
        (void)session->send(
            serve::Message::make_response(message.sequence(), 0, message.payload()));
    }
};
server->set_handler(server_handler);
server->start();

auto client = std::make_shared<serve::Ipc_client>(loop, path);
auto client_handler = std::make_shared<serve::Ipc_session::Handler>();
client_handler->on_connected = [](const serve::Ipc_session::Ptr& session) {
    std::vector<std::byte> payload{std::byte{'p'}, std::byte{'i'}, std::byte{'n'}, std::byte{'g'}};
    (void)session->send(serve::Message::make_request(1, 42, std::move(payload)));
};
client_handler->on_message = [](const serve::Ipc_session::Ptr&,
                                serve::Message& response) {
    // response.kind() == RESPONSE，response.sequence() 对应请求
};
client->set_handler(client_handler);
client->connect_async();
```

要点：

- 帧头 16 字节，`method`/`sequence`/`status` 由上层约定；`serve` 不维护方法表；
- 坏帧会直接断开连接，半包会等下一批；
- Windows 管道名必须以 `\\.\pipe\` 开头；POSIX 路径在 `start()` 前会被 `unlink`；
- Windows 下所有实例都被占用时，连接会按 10 ms 异步重试最多 50 次。

完整版本见 [ipc_ping_pong.cpp](../valley/serve/examples/ipc_ping_pong.cpp)。

## 6. RPC：双向请求-响应与通知

### 6.1 统一入口（推荐）

地址决定传输，代码不用改：

```cpp
auto server = std::make_shared<serve::Server>(loop, "ipc://valley_demo");
// 跨机器时换成：
// auto server = std::make_shared<serve::Server>(loop, "tcp://0.0.0.0:9000");

server->register_method(1, echo);
server->setup_max_sessions(256);      // 连接上限，0 = 不限制
server->setup_max_inflight(64);       // 每连接在飞请求上限
server->start();

auto client = std::make_shared<serve::Client>(loop, server->address());
client->request(1, payload, on_response);
client->connect_async();
```

下面的 6.2 是低级 API（需要直接控制传输细节时用）。

### 6.2 低级 API：rpc::Ipc_server / rpc::Ipc_client

`rpc::Ipc_server` / `rpc::Ipc_client` 对等：两边都能注册方法（给对方提供服务），也都能发起请求/通知。
内部是 `Registry`（方法表）+ `Peer`（一条连接上的序号关联与分发）。

```cpp
// 服务端：注册方法，同时也能调用客户端
auto server = std::make_shared<serve::Ipc_server>(loop, path);
auto rpc = std::make_shared<serve::rpc::Ipc_server>();
rpc->attach(loop, server);                 // 必须在 server->start() 之前

rpc->register_method(1, [](const serve::Ipc_session_ptr& session,
                           serve::Message& request,
                           std::vector<std::byte>& response) {
    response = request.payload();          // 原样回显
});
rpc->register_notification(7, [](const serve::Ipc_session_ptr&,
                                 serve::Message&,
                                 std::vector<std::byte>&) { /* 只收不回 */ });

std::uint64_t session_id = 0;
rpc->set_on_connected([&](const serve::Ipc_session_ptr& session) { session_id = session->id(); });
server->start();

// 连接侧：同样可以注册方法，供服务端调用
auto client = std::make_shared<serve::rpc::Ipc_client>(loop, path);
client->register_method(2, [](const serve::Ipc_session_ptr&,
                              serve::Message& request,
                              std::vector<std::byte>& response) {
    response = request.payload();
});
// 接收服务端通知：method 号必须与发送方一致
client->register_notification(8, [](const serve::Ipc_session_ptr&,
                                    serve::Message&,
                                    std::vector<std::byte>&) { /* ... */ });
client->set_error_handler([](const std::error_code& ec) {
    std::fprintf(stderr, "connect failed: %s\n", ec.message().c_str());
});
client->connect_async();

// 客户端 -> 服务端
client->request(1, payload,
    [](serve::rpc::Status status, std::vector<std::byte> response) {
        if (status == serve::rpc::Status::OK) {
            // 用 response
        }
    },
    std::chrono::milliseconds(500));

// 服务端 -> 客户端（session_id 来自 set_on_connected）
rpc->request(session_id, 2, payload,
    [](serve::rpc::Status status, std::vector<std::byte> response) { /* ... */ },
    std::chrono::milliseconds(500));

// 通知双向：接收方用 register_notification 注册对应 method
client->notify(7, payload);
rpc->notify(session_id, 8, payload);

// 广播（任意线程）：loop 线程内返回实际发送数，其它线程返回目标会话数快照
{
    (void)rpc->broadcast_notify(8, payload);
    (void)rpc->broadcast_request(
        2, payload,
        [](std::uint64_t session_id, serve::rpc::Status status,
           std::vector<std::byte> response) {
            // 每个会话各回调一次
        });
}
```

要点：

- 状态码见 `valley/serve/status.h`：`rpc::Status::OK`、`rpc::Status::UNIMPLEMENTED`、`rpc::Status::HANDLER_ERROR`、
  `rpc::Status::DEADLINE_EXCEEDED`、`rpc::Status::UNAVAILABLE`、`rpc::Status::RESOURCE_EXHAUSTED`、`rpc::Status::CONNECTION_CLOSED`；
- `request()` 返回 `true` 只代表"已受理"，失败走回调 `status`，不会静默丢弃；
- 超时后迟到的响应会被丢弃，不会二次回调；
- 断开时所有 pending 请求各回调一次 `rpc::Status::CONNECTION_CLOSED`；
- handler 抛异常 → 对端收到 `rpc::Status::HANDLER_ERROR`；
- 通知没有回包通道，异常被吞掉，需要错误可见就用 `request`；
- 两个方向的 `sequence` 各自独立，互不干扰；
- 连接建立后双方先交换能力表（HELLO），`on_connected` 在握手完成后才触发；
  调用未声明的 method 会被本地拒绝（`rpc::Status::UNIMPLEMENTED`），不会发出字节；
- 能力表是建连快照：握手之后新 `register_method` 的方法，对端要重连才能看到。

### TCP RPC

```cpp
// 服务端
auto server = std::make_shared<serve::rpc::Tcp_server>(loop, 9000);
server->register_method(1, echo);
server->setup_max_sessions(256);      // 连接上限，0 = 不限制
server->setup_max_inflight(64);       // 每连接在飞请求上限
server->start();

// 客户端（host 只接受 IP 字面量）
auto client = std::make_shared<serve::rpc::Tcp_client>(loop, "127.0.0.1", server->port());
client->request(1, payload, on_response);
client->connect_async();
```

与 IPC 版接口一一对应；内部自动开 `TCP_NODELAY`，没有 TLS/鉴权。

## 7. 延迟基准

```powershell
# 默认：两个 Event_loop / 两个线程，模拟对端进程
.\build_serve\Release\ipc_latency_bench.exe --rpc --samples 20000

# 纯 IPC，用于对比 RPC 层开销
.\build_serve\Release\ipc_latency_bench.exe --ipc --samples 20000

# 单 loop（同一线程），隔离线程唤醒成本
.\build_serve\Release\ipc_latency_bench.exe --rpc --same-loop

# 多连接（聚合吞吐）
.\build_serve\Release\ipc_latency_bench.exe --rpc --connections 4 --samples 10000

# payload 扫描（16B / 64B / 256B / 1KB / 4KB，RPC 与 IPC 各跑一遍）
.\build_serve\Release\ipc_latency_bench.exe --sweep
```

传输选择：默认走 IPC（命名管道 / 本机套接字），加 `--tcp` 改走 TCP，用于对比。
其它参数：`--warmup N`（默认 2000）、`--payload N`（默认 16）。输出里还有
`allocs/round`：每个往返的堆分配次数（基准替换了全局 `operator new` 计数），用来观察复用效果。基准本身在响应回调里直接发下一发，
不额外 `post`，避免把调度开销算进往返时延。

本机 Windows 命名管道、Release、16B payload 的实测（串行 ping-pong，单位 µs）：

| 场景 | 模式 | mean | p50 | p99 | p99.9 | 吞吐 |
|---|---|---:|---:|---:|---:|---:|
| 双线程双 loop | RPC | 20–21 | 20 | 48–59 | 100–120 | ~47k 往返/s |
| 双线程双 loop | IPC | 16–24 | 17–20 | 45–73 | 80–159 | ~41–62k 往返/s |
| 单 loop 单线程 | RPC | 7.4–7.7 | 7.0 | 15–17 | 42–45 | ~129–133k 往返/s |
| 单 loop 单线程 | IPC | 6.7–6.8 | 6.4 | 13–14 | 33–34 | ~145–149k 往返/s |
| WSL2 单 loop | RPC | 5.1 | 4.1 | 36 | 67 | ~196k 往返/s |
| WSL2 单 loop | IPC | 5.1 | 4.2 | 37 | 68 | ~195k 往返/s |

结论：

- **线程唤醒是大头**：双线程比单 loop 多约 13 µs，约占往返时延的三分之二；跨进程部署时这部分省不掉。
- **RPC 层开销约 0.7 µs**（单 loop：7.5 vs 6.8 µs），主要是序号表、能力校验和一次 payload 复制。
- **分配次数**：一次 RPC 往返 5 次堆分配（IPC 为 4 次），比优化前的 7 次少了响应 payload 拷贝
  和 pending 表节点；剩下的是双方各一次 `parse` 的 payload 复制，以及业务 handler 的回包拷贝。
- **payload 不敏感**：16B → 4KB 只增加约 0.5–3 µs，瓶颈在管道往返而不在拷贝。
- **并发**：4 连接聚合约 148k 往返/s（IPC 162k）；8 连接时服务端单 loop 饱和，时延升到 ~36 µs。

## 8. 定时器

```cpp
serve::Steady_timer timer(loop);
timer.expire_after([](bool expired) {
    if (expired)
        std::puts("tick");
}, std::chrono::milliseconds(100));
```

- 回调参数 `true` = 到期，`false` = 被 `cancel()`；
- 重复 `expire_*` 会覆盖旧等待，旧回调直接丢弃（不会再收到 `false`）；
- `expire_*` / `cancel()` 可以从任意线程调用。

见 [timer.cpp](../valley/serve/examples/timer.cpp)。

## 9. 线程与生命周期

| 操作 | 线程 |
|---|---|
| 回调（`on_connected` / `on_received` / `on_message` / 定时器） | 永远在 loop 线程 |
| `send` / `close` / 服务端 `start` `stop` `broadcast` | loop 线程 |
| `connect_async` / `disconnect` / `reconnect_async` / 定时器 `expire_*` `cancel` | 任意线程（内部投递） |
| `connected_sessions` / `find_session` / `is_connected` | 任意线程 |

生命周期：`Tcp_server` / `Tcp_client` / `Ipc_server` / `Ipc_client` / session 必须由
`std::shared_ptr` 托管；异步回调靠 `shared_from_this()` 续命，栈对象会悬空。
析构客户端走 `close_async()`，可在任意线程销毁；`Event_loop` 不要在自己的回调里销毁。

## 10. 生产接入清单

- **背压**：`send()` 返回 `false` 表示队列超限或未连接，必须处理（丢弃、限速或计数），
  不要忽略返回值；
- **超时**：连接、请求-响应都要用 `Steady_timer` 兜底，别让业务无限等；
- **错误**：`on_error` 里不要 `throw`，记录后断开即可；`operation_aborted` 是主动关闭的正常路径，不会上报；
- **优雅退出**：先 `client->disconnect()` / `server->stop()`（投递到 loop），再 `loop.stop()`，
  最后释放对象；
- **日志**：回调里只做轻量处理，重活扔到业务线程池，避免堵住 loop；
- **安全**（Windows）：命名管道默认 DACL 给 Everyone 放行，跨用户场景请自行收紧。

## 11. 常见坑

| 现象 | 原因 |
|---|---|
| 链接报 `ws2_32`/`mswsock` 未定义 | 没有 `target_link_libraries(... serve)`，asio 的依赖没传递到 |
| `server->start()` 返回 `false` | 地址非法（看 `address_error()`）、端口被占、或没有权限 |
| 客户端一直连不上，也没有回调 | 忘了 `connect_async()`，或 `on_error` 没注册导致失败被吞 |
| 回调里访问的对象已析构 | 用了栈对象或裸指针；必须 `shared_ptr` 托管 |
| 命名管道报 `Access is denied` | 服务端 DACL 限制（本库默认 Everyone；自建管道时需自己设安全描述符） |
| 收到数据不完整 | `on_received` 的 `consumed` 填错，或没按协议处理半包 |
| 发送数据重复/乱序 | 在 `on_sent` 里直接写 socket，绕过了缓冲队列；统一走 `session->send()` |
