# serve 接口

本文按头文件逐个列接口；设计取舍与线程模型见 [serve_design.md](serve_design.md)，接入步骤与示例见 [serve_usage.md](serve_usage.md)。
公开类型都在 `namespace serve` 下；IPC 的分帧/传输实现细节在 `namespace serve::ipc` 下，不属于对外接口。

## 0. 头文件与构建接入

| 头文件 | 内容 |
|---|---|
| [`valley/serve/serve.h`](../valley/serve/include/serve/serve.h) | 伞形头，包含下面全部 |
| [`valley/serve/server.h`](../valley/serve/include/serve/server.h) | `serve::Server`（统一入口，按地址选传输） |
| [`valley/serve/client.h`](../valley/serve/include/serve/client.h) | `serve::Client` |
| [`valley/serve/address.h`](../valley/serve/include/serve/address.h) | `serve::Address`、`serve::Transport` |
| [`valley/serve/event_loop.h`](../valley/serve/include/serve/event_loop.h) | `Event_loop` |
| [`valley/serve/steady_timer.h`](../valley/serve/include/serve/steady_timer.h) | `Steady_timer` |
| [`valley/serve/src/detail/buffered_stream.h`](../valley/serve/src/detail/buffered_stream.h) | `Buffered_stream<Stream, Derived>`（模板） |
| [`valley/serve/src/tcp/session.h`](../valley/serve/src/tcp/session.h) | `Tcp_session` |
| [`valley/serve/src/tcp/server.h`](../valley/serve/src/tcp/server.h) | `Tcp_server` |
| [`valley/serve/src/tcp/client.h`](../valley/serve/src/tcp/client.h) | `Tcp_client` |
| [`valley/serve/message.h`](../valley/serve/include/serve/message.h) | `serve::Message`、`serve::Kind` |
| [`valley/serve/session_base.h`](../valley/serve/include/serve/session_base.h) | `Session_base`（传输无关的分帧会话接口） |
| [`valley/serve/src/valley/ipc/session.h`](../valley/serve/src/valley/ipc/session.h) | `Ipc_session`（实现模板 `ipc::Basic_session<Stream>`） |
| [`valley/serve/src/valley/ipc/server.h`](../valley/serve/src/valley/ipc/server.h) | `Ipc_server` |
| [`valley/serve/src/valley/ipc/client.h`](../valley/serve/src/valley/ipc/client.h) | `Ipc_client` |
| [`valley/serve/src/valley/ipc/types.h`](../valley/serve/src/valley/ipc/types.h) | `Ipc_session` / `Ipc_session_ptr` 别名 |
| [`valley/serve/src/valley/ipc/transport.h`](../valley/serve/src/valley/ipc/transport.h) | `ipc::Transport_stream`（平台类型） |
| [`valley/serve/status.h`](../valley/serve/include/serve/status.h) | `rpc::Status::OK` / `rpc::Status::DEADLINE_EXCEEDED` 等状态码 |
| [`valley/serve/src/rpc/handshake.h`](../valley/serve/src/rpc/handshake.h) | `rpc::Capabilities`（握手能力表） |
| [`valley/serve/src/rpc/registry.h`](../valley/serve/src/rpc/registry.h) | `rpc::Registry`、`rpc::Handler` |
| [`valley/serve/src/rpc/peer.h`](../valley/serve/src/rpc/peer.h) | `rpc::Peer`（单连接的对等端） |
| [`valley/serve/src/rpc/valley/ipc/server.h`](../valley/serve/src/rpc/valley/ipc/server.h) | `rpc::Ipc_server` |
| [`valley/serve/src/rpc/valley/ipc/client.h`](../valley/serve/src/rpc/valley/ipc/client.h) | `rpc::Ipc_client` |
| [`valley/serve/src/rpc/tcp/client.h`](../valley/serve/src/rpc/tcp/client.h) | `rpc::Tcp_client`（TCP 传输） |
| [`valley/serve/src/rpc/tcp/frame_session.h`](../valley/serve/src/rpc/tcp/frame_session.h) | `rpc::Tcp_frame_session` 别名 |
| [`valley/serve/src/rpc/tcp/server.h`](../valley/serve/src/rpc/tcp/server.h) | `rpc::Tcp_server`（TCP 监听侧） |
| [`valley/serve/types.h`](../valley/serve/include/serve/types.h) | `serve::Handler` / `Response_handler` / `Broadcast_handler` |

> 公开头文件只有 `valley/serve/*.h`；下面标 `src/` 的是实现细节，不进公开安装集，列在这里方便维护。
> 需要直接使用 `Tcp_*` / `Ipc_*` / `rpc::*` 时，把 `valley/serve/src` 加进 include 路径即可。

CMake 接入：

```cmake
find_package(asio CONFIG REQUIRED)   # Conan；离线时回退 VALLEY_ASIO_INCLUDE
add_subdirectory(serve)                 # 或把 valley/serve/ 作为独立工程先构建
target_link_libraries(my_app PRIVATE serve)
```

`serve` 是静态库，`ASIO_STANDALONE`、asio 头文件目录与
`ws2_32/mswsock/advapi32` 都通过 `PUBLIC` 属性导出，消费者不用重复配置。

---

## 0.5 统一入口（推荐先看）

`serve::Server` / `serve::Client` 是唯一推荐的入口：构造时传一条地址，内部自动选择 IPC 或 TCP，
接口与 `rpc::Ipc_server` / `rpc::Ipc_client` 一致。地址由 `serve::Address::parse()` 解析：

| 地址 | 含义 |
|---|---|
| `ipc://name` | Windows `\\.\pipe\name`；POSIX `/tmp/name` |
| `ipc:///tmp/x.sock` | POSIX 绝对路径（Windows 拒绝） |
| `tcp://127.0.0.1:9000` | IPv4 |
| `tcp://[::1]:9000` | IPv6 |
| `tcp://0.0.0.0:0` | 显式监听所有地址 + 系统分配端口（客户端拒绝通配地址） |

```cpp
serve::Server server(loop, "ipc://valley_demo");     // 或 "tcp://0.0.0.0:9000"
server.register_method(1, echo);
server.start();

serve::Client client(loop, server.address());        // TCP 端口 0 时 address() 已回填
client.request(1, payload, on_response);
client.connect_async();                            // 或 connect_async(2000ms) 带超时
```

- 地址非法 → 构造函数抛 `std::invalid_argument`；
- bind 失败 → `start()` 返回 `false`，原因在 `address_error()`；
- 其余语义（握手、超时、双向、广播、线程约定）与下面的 RPC 章节完全一致；
- 完整设计与实现方案见 [serve_facade.md](serve_facade.md)。

## 1. `Event_loop`

`asio::io_context` 的薄封装，所有回调都在 loop 线程执行。**公开头文件不包含 asio**：

```cpp
enum class Mode { FOREGROUND, BACKGROUND };

explicit Event_loop(std::string name = "loop", Mode mode = Mode::BACKGROUND);
~Event_loop();                                    // 内部 stop()
Event_loop(const Event_loop&) = delete;           // 拷贝/移动均删除
```

| 方法 | 说明 |
|---|---|
| `void start()` | 后台模式起线程后返回；前台模式在调用线程阻塞运行 |
| `void stop()` | 停循环并回收线程；可在任意线程调用（loop 线程内只 detach） |
| `void restart()` | `stop()` 之后重新允许驱动 |
| `std::size_t poll()` | 非阻塞跑一轮，返回处理的事件数 |
| `bool poll_one()` | 非阻塞处理一个事件 |
| `std::size_t run_for(std::chrono::milliseconds)` | 最多跑指定时长 |
| `void post(std::function<void()>)` | 投递任务（总在之后的某一轮执行） |
| `void dispatch(std::function<void()>)` | 已在 loop 线程则内联执行，否则等价 post |
| `bool is_in_loop_thread() const` | 当前线程是否正在跑该 loop |
| `bool is_running() const` | 循环是否在运行 |
| `asio::io_context& context()` | 裸上下文（给 asio 对象构造用） |
| `asio::io_context::executor_type executor()` | 构造 asio 对象时的 executor |
| `static Event_loop& default_loop()` | 进程内常驻后台循环，首次调用时启动 |

需要 `io_context` 只能来自实现层：`valley/serve/src/detail/event_loop_internal.h` 提供
`serve::detail::Event_loop_internal::context(loop)` / `executor(loop)`，不属于公开接口。

```cpp
serve::Event_loop loop("worker");
loop.start();

loop.post([&loop] {
    // 这里在 loop 线程
    loop.dispatch([] { /* 内联执行 */ });
});

loop.stop();
```

约束：`poll()/poll_one()/run_for()` 只用于手动驱动，不要与 `start()` 的后台线程混用；
不要在 loop 线程里销毁 `Event_loop`。

---

## 2. `Steady_timer`

单次定时器，回调在 loop 线程触发。

```cpp
explicit Steady_timer(Event_loop& loop);
~Steady_timer();                                  // 内部 cancel()

void expire_after(std::function<void(bool)> action, std::chrono::milliseconds span);
void expire_at(std::function<void(bool)> action, std::chrono::steady_clock::time_point tp);
void cancel();
bool is_pending() const noexcept;
```

- `action(true)`：到期触发；
- `action(false)`：被 `cancel()` 取消；
- 重复 `expire_*` 会覆盖上一次等待，旧回调直接丢弃（不会再收到 `false`）；
- `expire_*` 与 `cancel()` 可从任意线程调用，内部投递到 loop。

```cpp
serve::Steady_timer timer(loop);
timer.expire_after([&](bool expired) {
    if (expired) { /* 到点 */ }
}, std::chrono::milliseconds(50));
```

---

## 3. `Buffered_stream<Stream, Derived>`

有界收发缓冲，TCP 与 IPC 共用。一般不直接实例化，而是作为 `Tcp_session` /
`Ipc_session` 的基类使用。

```cpp
using stream_type   = Stream;
using executor_type = typename Stream::executor_type;

enum class State { IDLE, CONNECTED, CLOSING, CLOSED };

static constexpr std::size_t RECEIVE_CHUNK         = 64 * 1024;
static constexpr std::size_t DEFAULT_RECEIVE_LIMIT = 4 * 1024 * 1024;
static constexpr std::size_t DEFAULT_SEND_LIMIT    = 16 * 1024 * 1024;

explicit Buffered_stream(executor_type executor);
```

| 方法 | 说明 |
|---|---|
| `bool send(const void* data, std::size_t size)` | 拷贝入队；未连接、`size == 0` 或超过 `send_limit` 返回 `false` |
| `Stream& next_layer()` | 底层 asio 流/句柄 |
| `State state() const` | 当前状态 |
| `bool is_connected() const` / `bool is_closing() const` | 状态便捷判断 |
| `std::size_t bytes_pending() const` | 已入队未发完的字节数 |
| `std::uint64_t bytes_sent() const` / `bytes_received() const` | 累计字节数 |
| `void setup_send_limit(std::size_t)` / `setup_receive_limit(std::size_t)` | 改背压/缓冲上限 |
| `void disconnect()` / `void disconnect_async()`（保护） | 关闭连接；后者投递到 loop |
| `void start()`（保护） | 进入 CONNECTED 并开始读，由派生类在连接建立后调用 |

派生类（`Derived`）必须实现以下回调，并由 `Buffered_stream` 通过 CRTP 调用：

| 回调 | 时机 |
|---|---|
| `void on_connected()` | `start()` 之后、第一次读之前 |
| `void on_disconnected()` | 连接关闭，只调一次 |
| `void on_received(const void* data, std::size_t size, std::size_t& consumed)` | 每次读完成后；`consumed` 表示本次消费多少，剩余留在缓冲 |
| `void on_sent(std::size_t sent, std::size_t pending)` | 每写完一块 |
| `void on_empty()` | 发送队列清空 |
| `void on_error(const std::error_code&)` | 读/写失败（`operation_aborted` 除外） |

另外派生类必须提供 `shared_from_this()`（异步 handler 用它续命）。

---

## 4. `Tcp_session`

一条已建立的 TCP 连接。

```cpp
struct Handler
{
    std::function<void(const Ptr&)> on_connected;
    std::function<void(const Ptr&)> on_disconnected;
    std::function<void(const Ptr&, const void* data, std::size_t size, std::size_t& consumed)> on_received;
    std::function<void(const Ptr&, std::size_t sent, std::size_t pending)> on_sent;
    std::function<void(const Ptr&)> on_empty;
    std::function<void(const Ptr&, const std::error_code&)> on_error;
};

using Ptr = std::shared_ptr<Tcp_session>;

Tcp_session(asio::ip::tcp::socket socket, std::uint64_t id);
```

| 方法 | 说明 |
|---|---|
| `std::uint64_t id() const` | 服务端分配的会话号 |
| `asio::ip::tcp::socket& socket()` | 底层 socket |
| `std::string remote_address() const` | 对端 IP 文本；取不到返回空串 |
| `void open()` / `void close()` / `void close_async()` | 启动 / 关闭（同步）/ 关闭（投递到 loop） |
| `bool send(const void* data, std::size_t size)` | 见 `Buffered_stream::send` |
| `void set_handler(std::shared_ptr<const Handler>)` | 安装回调集合 |
| `void set_close_handler(std::function<void(std::uint64_t)>)` | 关闭后回调，服务端用它摘表 |

`consumed` 语义：默认等于 `size`；只处理了前 N 字节就填 N，剩余字节留在缓冲里等下次回调。

---

## 5. `Tcp_server`

```cpp
using Session     = Tcp_session;
using Session_ptr = std::shared_ptr<Tcp_session>;

Tcp_server(Event_loop& loop, int port, bool ipv4 = true);   // 默认监听 127.0.0.1 / ::1
Tcp_server(Event_loop& loop, std::string address, int port);
```

| 方法 | 说明 |
|---|---|
| `bool start()` | `open/bind/listen` 并进入 accept 循环；失败返回 `false` |
| `void stop()` | 关闭监听、断开所有会话（幂等） |
| `bool is_started() const` | 是否在监听 |
| `int port() const` | 实际端口（构造时传 0 会在 `start()` 后回填） |
| `const std::string& address() const` | 监听地址 |
| `std::size_t connected_sessions() const` | 当前会话数（线程安全） |
| `bool broadcast(const void* data, std::size_t size)` | 向所有会话发送，至少一个成功即 `true` |
| `bool disconnect_all()` | 断开所有会话 |
| `Session_ptr find_session(std::uint64_t id) const` | 按 id 查会话（线程安全） |
| `void setup_keep_alive(bool)` / `setup_no_delay(bool)` / `setup_reuse_address(bool)` / `setup_reuse_port(bool)` | 套接字选项；`reuse_port` 仅在定义了 `SO_REUSEPORT` 的平台生效 |
| `void set_handler(std::shared_ptr<const Tcp_session::Handler>)` | 新会话统一使用的回调 |

约束：必须由 `std::shared_ptr` 托管；`start/broadcast/disconnect_all` 在 loop 线程调用；`stop()` 任意线程（非 loop 线程会投递并等待关闭完成）。
构造时地址非法不会抛异常：`address_error()` 返回非空的 `std::error_code`，`start()` 直接返回 `false`。

---

## 6. `Tcp_client`

```cpp
Tcp_client(Event_loop& loop, std::string address, int port);
```

| 方法 | 说明 |
|---|---|
| `void connect_async()` | 异步连接；可跨线程 |
| `void disconnect()` | 主动断开；可跨线程 |
| `void reconnect_async()` | 置重连标志，断开后自动重连 |
| `bool send(const void* data, std::size_t size)` | loop 线程调用；未连接返回 `false` |
| `bool is_connected() const` / `bool is_connecting() const` | 状态查询 |
| `Session_ptr session() const` | 当前会话（仅 loop 线程可读） |
| `bool address_error() const` | 地址解析结果；非空时 `connect_async()` 直接回调 error handler |
| `void setup_keep_alive(bool)` / `setup_no_delay(bool)` | 连接建立时套用 |
| `void set_handler(std::shared_ptr<const Tcp_session::Handler>)` | 会话回调 |
| `void set_error_handler(std::function<void(const std::error_code&)>)` | 连接失败上报（没有会话可挂在 `on_error` 上） |

约束：必须由 `std::shared_ptr` 托管；析构走 `close_async()`，可在任意线程销毁。

---

## 7. `serve::Message`（IPC / TCP 共用）

IPC 帧的构造与编解码。

```cpp
enum class Kind : std::uint16_t
{
    REQUEST = 1, RESPONSE = 2, NOTIFICATION = 3,
    HELLO = 4, PING = 5, PONG = 6
};

static constexpr std::size_t HEADER_SIZE = 16;   // 帧头不带 magic
static constexpr std::size_t MAX_PAYLOAD = 4 * 1024 * 1024;

static Message make_request(std::uint16_t method, std::uint32_t sequence,
                            std::vector<std::byte> payload = {});
static Message make_response(std::uint32_t sequence, std::uint32_t status,
                             std::vector<std::byte> payload = {});
static Message make_notification(std::uint16_t method,
                                 std::vector<std::byte> payload = {});
static Message make_hello(std::vector<std::byte> payload);
static Message make_ping(std::uint32_t sequence);
static Message make_pong(std::uint32_t sequence);

Kind kind() const;  std::uint16_t method() const;
std::uint32_t sequence() const;  std::uint32_t status() const;
const std::vector<std::byte>& payload() const;
std::vector<std::byte>& payload() noexcept;   // 响应处理时可 move 走

std::vector<std::byte> serialize() const;
std::size_t frame_size() const;
void serialize_into(std::byte* dest) const;
static std::optional<Message> parse(const std::byte* data, std::size_t size,
                                    std::size_t& consumed);
static bool is_valid_header(const std::byte* data, std::size_t size) noexcept;
```

- `parse`：数据不足或格式非法返回 `nullopt` 且 `consumed = 0`；成功时 `consumed` 为整帧长度；
- `is_valid_header`：单独判断 kind/size，区分"半包"与"坏帧"；HELLO 载荷里的 magic 与协议版本只在 RPC 握手时校验一次。

---

## 8. `Ipc_session`（`ipc::Basic_session<Stream>`）

分帧会话，`Ipc_session` 是它在平台流类型上的别名。

```cpp
struct Handler
{
    std::function<void(const Ptr&)> on_connected;
    std::function<void(const Ptr&)> on_disconnected;
    std::function<void(const Ptr&, Message&)> on_message;   // 非 const：上层可 move 走 payload
    std::function<void(const Ptr&, const std::error_code&)> on_error;
};

Basic_session(Stream stream, std::uint64_t id);
```

| 方法 | 说明 |
|---|---|
| `std::uint64_t id() const` | 会话号 |
| `Stream& next_layer()` | 底层管道/套接字 |
| `void open()` / `void close()` / `void close_async()` | 启动 / 关闭 |
| `bool send(const Message&)` | 序列化后入队，走 `Buffered_stream::send` 的背压 |
| `void set_handler(std::shared_ptr<const Handler>)` | 回调集合 |
| `void set_close_handler(std::function<void(std::uint64_t)>)` | 关闭后回调 |

`on_message` 每解析出一个完整帧调用一次；帧以非 const 引用传入，上层可以 `std::move(payload())`
拿走数据，避免多一次拷贝。坏帧按协议错误断开连接。

---

## 9. `Ipc_server` / `Ipc_client`

```cpp
// Server
Server(Event_loop& loop, std::string path);
bool start();  void stop();
const std::string& path() const;
bool is_started() const;  std::size_t connected_sessions() const;
bool send(std::uint64_t session_id, const Message& message);
bool broadcast(const Message& message);
void set_handler(std::shared_ptr<const Session::Handler>);

// Client
Client(Event_loop& loop, std::string path);
void connect_async();  void disconnect();
bool is_connected() const;  bool is_connecting() const;
bool send(const Message& message);
void set_handler(std::shared_ptr<const Session::Handler>);
void set_error_handler(std::function<void(const std::error_code&)>);
```

- `path`：Windows 为 `\\.\pipe\<name>`，POSIX 为文件系统路径（`start()` 前会 `unlink`）；
- Windows 下管道实例被占满（`ERROR_PIPE_BUSY`）时按 10 ms 异步重试，最多 50 次，不阻塞 loop；
- `Server::start/stop/send/broadcast` 与 `Session::send` 在 loop 线程调用；
  `Client::connect_async/disconnect` 可跨线程；
- 两个类都必须由 `std::shared_ptr` 托管，析构走 `close_async()`，可在任意线程销毁。

---

## 10. `rpc::Ipc_server` / `rpc::Ipc_client`

在 `serve::Message` 之上的薄 RPC 层，**两端对等**：都能 `register_method` / `register_notification`
（给对方提供服务），也都能 `request` / `notify`（调用对方）。状态码是 `enum class Status : std::int32_t`
（`valley/serve/status.h`）：0 成功，1..23 框架保留，>= 100 留给业务（`Status::USER`）。

| 值 | 枚举 | 含义 |
|---:|---|---|
| 0 | `Status::OK` | 成功 |
| 1 | `Status::CANCELLED` | 被取消 |
| 2 | `Status::UNKNOWN` | 未知错误 |
| 3 | `Status::INVALID_ARGUMENT` | 参数非法 |
| 4 | `Status::DEADLINE_EXCEEDED` | 超时（框架用于请求超时） |
| 5 | `Status::NOT_FOUND` | 对象不存在 |
| 6 | `Status::ALREADY_EXISTS` | 已存在 |
| 7 | `Status::PERMISSION_DENIED` | 无权限 |
| 8 | `Status::RESOURCE_EXHAUSTED` | 资源耗尽（框架用于发送队列拒绝） |
| 9 | `Status::FAILED_PRECONDITION` | 前置条件不满足 |
| 10 | `Status::ABORTED` | 中止 |
| 11 | `Status::OUT_OF_RANGE` | 越界 |
| 12 | `Status::UNIMPLEMENTED` | 对端没实现（框架用于方法未注册/未声明） |
| 13 | `Status::INTERNAL` | 内部错误 |
| 14 | `Status::UNAVAILABLE` | 暂不可用（框架用于未连接/握手未完成） |
| 15 | `Status::DATA_LOSS` | 数据丢失 |
| 16 | `Status::UNAUTHENTICATED` | 未认证 |
| 17 | `Status::IO` | I/O 错误 |
| 18 | `Status::CONNECTION_CLOSED` | 连接断开（框架用于等待期间的断开） |
| 19 | `Status::RECONNECT_FAILED` | 重连失败 |
| 20 | `Status::MAX_CONCURRENT` | 并发超限 |
| 21 | `Status::PROTOCOL` | 协议错误 |
| 22 | `Status::HANDLER_ERROR` | 业务 handler 抛异常 |
| 23 | `Status::BUSY` | 忙 |
| 100 | `Status::USER` | 业务自定义状态的起点 |

提供 `rpc::to_string(Status)` 与 `rpc::is_ok(Status)`；线上仍是 u32，跨线按 `int32_t` 数值转换。

实现上分成三层：`Registry`（方法表，两端各一份）、`Peer`（一条连接上的握手、请求关联与分发）、
`Server` / `Client`（监听侧 / 连接侧的封装）。

### 保活

`setup_keepalive(interval, timeout)` 打开应用层保活：每 `interval` 发一帧 `serve::Kind::PING`，
对端原样回填 sequence 的 `PONG`；超过 `timeout` 没收到任何入站数据就关闭会话。默认关闭，
`interval <= 0` 也表示关闭。

### 握手

连接建立后，双方的 `Peer` 各发一帧 `serve::Kind::HELLO`，payload 是本端当前已注册的
method id 与 notification id 列表（格式见 `valley/serve/src/rpc/handshake.h`）。收到对方的能力表后
`Peer` 才进入 ready 状态：

- `on_connected` 在**握手完成后**才触发，所以回调里可以直接调用对端；
- `request()` / `notify()` 会用对端声明的能力表做本地校验：没声明 → 回调
  `rpc::Status::UNIMPLEMENTED`，不发出字节；握手未完成 → `rpc::Status::UNAVAILABLE`；
- `register_method` / `register_notification` 成功后会给已连接的对端补发一次 HELLO，对端据此刷新能力表；在连接前注册则随首次 HELLO 一起发送。

### `rpc::Ipc_server`（监听侧）

```cpp
void attach(Event_loop& loop, const std::shared_ptr<Ipc_server>& server);  // 装分发器

bool register_method(std::uint16_t method, Handler handler);
bool register_notification(std::uint16_t method, Handler handler);

// 主动调用某个已连接的对端（session_id 来自 on_connected / find_session）
bool request(std::uint64_t session_id, std::uint16_t method,
             std::vector<std::byte> payload, Response_handler on_response,
             std::chrono::milliseconds timeout = std::chrono::seconds(3),
             bool replace_inflight = false);
bool notify(std::uint64_t session_id, std::uint16_t method, std::vector<std::byte> payload);

// 广播：逐个会话发送；loop 线程内返回实际发送数，其它线程返回目标会话数快照
using Broadcast_handler =
    std::function<void(std::uint64_t session_id, rpc::Status status,
                       std::vector<std::byte> payload)>;
std::size_t broadcast_notify(std::uint16_t method, std::vector<std::byte> payload);
std::size_t broadcast_request(std::uint16_t method, std::vector<std::byte> payload,
                              Broadcast_handler on_response,
                              std::chrono::milliseconds timeout = std::chrono::seconds(3));

std::size_t connected_sessions() const;
bool is_ready(std::uint64_t session_id) const;   // 该会话的握手是否完成
void set_on_connected(std::function<void(const Ipc_session_ptr&)>);
void set_on_disconnected(std::function<void(const Ipc_session_ptr&)>);
```

### `rpc::Ipc_client`（连接侧）

```cpp
rpc::Ipc_client(Event_loop& loop, std::string path);
void connect_async();
void connect_async(std::chrono::milliseconds timeout);   // 超时报 std::errc::timed_out
void disconnect();
bool is_connected() const;
bool is_ready() const;   // 握手是否完成

// 与 Server 对等：客户端也可以给服务端提供服务
bool register_method(std::uint16_t method, Handler handler);
bool register_notification(std::uint16_t method, Handler handler);

bool request(std::uint16_t method, std::vector<std::byte> payload,
             Response_handler on_response,
             std::chrono::milliseconds timeout = std::chrono::seconds(3),
             bool replace_inflight = false);
// timeout <= 0 表示不设超时；> 0 时到点回调 DEADLINE_EXCEEDED
bool notify(std::uint16_t method, std::vector<std::byte> payload);

std::uint64_t session_id() const;   // 仅 loop 线程；0 = 未连接
void setup_max_inflight(std::size_t limit) noexcept;   // 在飞请求上限；0 = 不限制
void set_on_connected(std::function<void()>);
void set_on_disconnected(std::function<void()>);
void set_error_handler(std::function<void(const std::error_code&)>);
```

### `rpc::Tcp_client`（TCP 传输）

```cpp
rpc::Tcp_client(Event_loop& loop, std::string host, int port);   // host 只接受 IP 字面量
void connect_async();  void disconnect();
bool is_connected() const;  bool is_ready() const;
bool register_method(std::uint16_t method, Handler handler);
bool register_notification(std::uint16_t method, Handler handler);
bool request(std::uint16_t method, std::vector<std::byte> payload,
             Response_handler on_response, std::chrono::milliseconds timeout = 3s,
             bool replace_inflight = false);
bool notify(std::uint16_t method, std::vector<std::byte> payload);
std::uint64_t session_id() const;   // 仅 loop 线程；0 = 未连接
void setup_max_inflight(std::size_t limit) noexcept;   // 在飞请求上限；0 = 不限制
void set_on_connected(std::function<void()>);
void set_on_disconnected(std::function<void()>);
void set_error_handler(std::function<void(const std::error_code&)>);
```

与 `rpc::Ipc_client` 语义一致（握手、能力校验、超时、迟到丢弃），差别只在传输：
内部是 `rpc::Tcp_frame_session = ipc::Basic_session<asio::ip::tcp::socket>`，连接成功后自动开
`TCP_NODELAY`。`host` 目前只接受 IP 字面量（不做 DNS）；没有 TLS/鉴权，暴露到公网前要自己加。

### `rpc::Tcp_server`（TCP 传输，监听侧）

```cpp
rpc::Tcp_server(Event_loop& loop, int port, bool ipv4 = true);   // 默认 loopback；port 0 = 系统分配
rpc::Tcp_server(Event_loop& loop, std::string bind_address, int port, bool ipv4 = true);
bool start();  void stop();  bool is_started() const;
    // 优雅关闭：停止接收新连接，等在飞请求完成（或超时）再关会话
    void stop_graceful(std::chrono::milliseconds timeout, std::function<void()> on_done = {});
const std::string& address() const;  int port() const;
std::size_t connected_sessions() const;
bool is_ready(std::uint64_t session_id) const;

bool register_method(std::uint16_t method, Handler handler);
bool register_notification(std::uint16_t method, Handler handler);

bool request(std::uint64_t session_id, std::uint16_t method, std::vector<std::byte> payload,
             Response_handler on_response, std::chrono::milliseconds timeout = 3s,
             bool replace_inflight = false);
bool notify(std::uint64_t session_id, std::uint16_t method, std::vector<std::byte> payload);
std::size_t broadcast_notify(std::uint16_t method, std::vector<std::byte> payload);
std::size_t broadcast_request(std::uint16_t method, std::vector<std::byte> payload,
                              Broadcast_handler on_response,
                              std::chrono::milliseconds timeout = 3s);

void set_on_connected(std::function<void(const Session_ptr&)>);
void set_on_disconnected(std::function<void(const Session_ptr&)>);
void setup_no_delay(bool enable);   // 默认 true
```

与 `rpc::Ipc_server` 的差别同样只在传输：每 accept 一个 socket 就建一个 `Tcp_frame_session`，
握手完成后再回调 `on_connected`。`start()` 在 loop 线程（或 loop 启动前）调用；`stop()` / `stop_graceful()` 任意线程（`stop()` 跨线程会等待关闭完成），
`broadcast_*` 可从任意线程调用：loop 线程内直接发送，其它线程投递后立即返回（返回值是目标会话数的快照）。没有 TLS/鉴权；`tcp://` 服务端按地址 host 绑定（`0.0.0.0`/`::` 才是对外），暴露前请自行限制。

### 共同约束

- `Handler` 与 `Response_handler` 定义在 `valley/serve/src/rpc/registry.h` / `valley/serve/src/rpc/peer.h`；
- method 0 保留；重复注册返回 `false`；
- 未注册的 `REQUEST` 自动回 `rpc::Status::UNIMPLEMENTED`；handler 抛异常回 `rpc::Status::HANDLER_ERROR`；
- `NOTIFICATION` 没有回包通道，异常被吞掉（需要错误可见就用 `request`）；
- 握手前 `request/notify` 会被本地拒绝（`rpc::Status::UNAVAILABLE`），不会发出字节；
- `request()` 返回 `true` 只表示"已受理"；失败通过回调 `status` 上报
  （`rpc::Status::UNAVAILABLE` / `rpc::Status::RESOURCE_EXHAUSTED` / `rpc::Status::DEADLINE_EXCEEDED` / `rpc::Status::CONNECTION_CLOSED`）；
- 超时后迟到的响应按 `sequence` 丢弃，不会二次回调；
- 连接断开时，所有 pending 请求各回调一次 `rpc::Status::CONNECTION_CLOSED`；
- 同一连接上每个 method 只允许一个在飞请求：重复调用立刻回调 `rpc::Status::BUSY`，不发出字节；
  `request(..., replace_inflight = true)` 会先用 `rpc::Status::CANCELLED` 结束旧请求再发新的
  （旧请求只是本地丢弃，对端仍可能执行完，协议里没有 CANCEL 帧）；每个 method 的 `sequence` 独立递增；
- 在 loop 线程内调用 `request()` 且当前未连接时，回调会同步触发（先回调、后返回）；
  从其他线程调用则一定异步；
- `broadcast_*` 可从任意线程调用；在 loop 线程内返回实际发送数，其它线程返回目标会话数快照（实际发送异步完成）；
- `Server::attach()` 只生效一次，重复调用直接返回；`attach()` 之前 `request/notify` 返回 `false`
  （还没拿到 loop），但 `register_method/register_notification` 可以先于 attach 调用；
- 析构 `Client`（以及析构 `Server` 时清空 `peers_`）不会再回调 pending 请求，
  需要收尾就显式 `disconnect()` 并等待 `rpc::Status::CONNECTION_CLOSED`；
- `register_*` / `connect_async` / `disconnect` 可跨线程，`request/notify` 也可跨线程（内部投递），
  所有回调都在 loop 线程；
- `attach()` 必须在 `server->start()` 之前调用，且两个类都必须 `shared_ptr` 托管。

## 11. 示例

### 11.1 TCP 回声服务

```cpp
serve::Event_loop loop("tcp");
loop.start();

auto handler = std::make_shared<serve::Tcp_session::Handler>();
handler->on_received = [](const serve::Tcp_session::Ptr& session, const void* data,
                          std::size_t size, std::size_t& consumed) {
    (void)session->send(data, size);
    consumed = size;
};

auto server = std::make_shared<serve::Tcp_server>(loop, 0, true);   // 0 = 系统分配端口
server->set_handler(handler);
server->start();
const int port = server->port();
```

### 11.2 定时器

```cpp
serve::Steady_timer timer(loop);
timer.expire_after([](bool expired) {
    if (expired)
        std::puts("tick");
}, std::chrono::milliseconds(100));
```

### 11.3 IPC 请求-响应

```cpp
auto server = std::make_shared<serve::Ipc_server>(loop, "\\\\.\\pipe\\demo");
auto server_handler = std::make_shared<serve::Ipc_session::Handler>();
server_handler->on_message = [](const serve::Ipc_session::Ptr& session,
                                serve::Message& request) {
    if (request.kind() == serve::Kind::REQUEST) {
        (void)session->send(
            serve::Message::make_response(request.sequence(), 0, request.payload()));
    }
};
server->set_handler(server_handler);
server->start();

auto client = std::make_shared<serve::Ipc_client>(loop, "\\\\.\\pipe\\demo");
auto client_handler = std::make_shared<serve::Ipc_session::Handler>();
client_handler->on_connected = [](const serve::Ipc_session::Ptr& session) {
    std::vector<std::byte> payload{std::byte{'p'}, std::byte{'i'}, std::byte{'n'}, std::byte{'g'}};
    (void)session->send(serve::Message::make_request(1, 42, std::move(payload)));
};
client_handler->on_message = [](const serve::Ipc_session::Ptr&,
                                serve::Message& response) {
    // response.kind() == RESPONSE，response.status()、response.payload()
};
client->set_handler(client_handler);
client->connect_async();
```

## 12. 速查：错误与线程约束

| 现象 | 语义 |
|---|---|
| `send()` 返回 `false` | 未连接、`size == 0`，或超过 `send_limit`（背压） |
| 连接失败 | `Tcp_client::set_error_handler` / `Ipc_client::set_error_handler` 上报 |
| 会话错误 | `Tcp_session::Handler::on_error`；`operation_aborted` 不上报 |
| 坏帧 | `Ipc_session` 断开连接（`std::errc::protocol_error`） |
| 接收缓冲到上限 | 断开连接（`std::errc::no_buffer_space`） |
| 回调线程 | 永远是 loop 线程 |
| 对象生命周期 | server/client/session 必须 `std::shared_ptr` 托管 |

## 13. 安装与 find_package

```bash
cmake --install .build_serve --config Release --prefix /opt/valley
```

```cmake
find_package(asio CONFIG REQUIRED)   # Conan；离线时回退 VALLEY_ASIO_INCLUDE
find_package(serve REQUIRED)
target_link_libraries(my_app PRIVATE valley::serve)
```

公开头文件不包含 asio，使用方不需要 asio 头文件；只有直接使用 `valley/serve/src` 下的实现层头时才需要自行提供 asio。
MSVC 使用方会自动拿到 `/utf-8`，否则头文件里的中文注释会被按 936 代码页误读。
