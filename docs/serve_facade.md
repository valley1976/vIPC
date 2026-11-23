# serve 统一入口设计：Server / Client

> 状态：已实现（方案 A：门面 + backend 适配），见 `valley/serve/src/server.cpp` / `client.cpp`。
> 目标：外部只见 `serve::Server` / `serve::Client` 和一条地址字符串，
> IPC 与 TCP 完全隐藏；地址形如 `ipc://name`、`tcp://host:port`。

## 1. 目标与非目标

目标：

- 对外只公开两个类：`serve::Server`、`serve::Client`；
- 构造时传一条地址，内部据此选择 IPC 或 TCP 实现；
- 两条传输的 RPC 语义完全一致（握手、方法表、请求-响应、通知、广播、超时）；
- 同一段测试代码换一条地址就能在 IPC / TCP 上跑；
- 保持现有线程约定：回调都在 loop 线程，`request/notify` 可从任意线程调用。

非目标：

- 不做 DNS（地址里只接受 IP 字面量）；
- 不做 TLS / 鉴权（跨机部署需要时在上一层加，或在 `Server` / `Client` 之上包一层 TLS 会话）；
- 不做地址里的查询参数（`?timeout=` 之类）；
- 不删除现有的 `Ipc_*` / `rpc::*` / `Tcp_*` 类——它们降级为「低级 API」，高级场景仍可直接用。

## 2. 命名说明

顶层就叫 `serve::Server` / `serve::Client`（单字类型名，符合仓库的 `Snake_case` 类型规范）：

- 它们是这个库**唯一推荐的入口**，名字应该最短；
- 传输相关的实现都带前缀：`serve::Tcp_server`、`serve::Ipc_server`、`serve::rpc::Ipc_server`，
  和顶层的 `serve::Server` 在名字上就能区分「高级门面 / 低级实现」；
- `serve::Server` 没有用 `Message_` 前缀：加前缀反而会让它看起来像又一个平级实现，
  而它其实是门面。文档里只保留一条规则：**新代码用 `serve::Server` / `serve::Client`，
  需要控制特定传输细节时才下沉到 `Tcp_*` / `Ipc_*` / `rpc::*`。**

## 3. 地址语法

```
address   := "ipc://" ipc-target | "tcp://" host ":" port
ipc-target:= <平台相关>
host      := IPv4 字面量 | "[" IPv6 字面量 "]"
port      := 0..65535        ; 服务端 0 = 系统分配
```

| 地址 | 含义 |
|---|---|
| `ipc://valley_service` | Windows：`\\.\pipe\valley_service`；POSIX：`/tmp/valley_service`（相对名会补 `/tmp/`） |
| `ipc:///tmp/valley.sock` | POSIX 绝对路径；Windows 上 `/tmp/...` 视为非法（Windows 只接受名字） |
| `tcp://127.0.0.1:9000` | IPv4 |
| `tcp://0.0.0.0:0` | IPv4 通配 + 系统分配端口（仅服务端） |
| `tcp://[::1]:9000` | IPv6 |

规则：

- scheme 只认 `ipc` 与 `tcp`，大小写敏感；其余一律 `std::invalid_argument`；
- `tcp` 的 host 必须是 IP 字面量（`asio::ip::make_address` 校验），不接受域名；
- 服务端允许 `0.0.0.0` / `::` 与端口 0；客户端拒绝通配地址；
- 解析结果统一规范化，`address()` 返回规范化后的字符串（tcp 端口 0 会回填实际端口）。

解析 API（可独立使用，便于配置层提前校验）：

```cpp
namespace serve {

enum class Transport { IPC, TCP };

struct Address
{
    Transport transport{Transport::IPC};
    std::string target;   // ipc: 路径/名字；tcp: host
    std::uint16_t port{0}; // 仅 tcp

    [[nodiscard]] static std::optional<Address> parse(std::string_view text) noexcept;
    [[nodiscard]] std::string to_string() const;
};

}  // namespace serve
```

## 4. 对外接口

### 4.1 Server

```cpp
class Server : public std::enable_shared_from_this<Server>
{
public:
    using Session_ptr      = Session_base::Ptr;
    using Handler          = rpc::Handler;            // (session, request, response&)
    using Response_handler = rpc::Peer::Response_handler;  // (Status, payload)
    using Broadcast_handler =
        std::function<void(std::uint64_t session_id, rpc::Status status,
                           std::vector<std::byte> payload)>;

    // 地址非法抛 std::invalid_argument；地址语法错误在构造期就暴露
    Server(Event_loop& loop, std::string address);

    [[nodiscard]] bool start();          // loop 线程或 loop 启动前
    void stop();
    [[nodiscard]] bool is_started() const noexcept;

    [[nodiscard]] std::string address() const;             // 规范化 + 端口回填
    [[nodiscard]] int port() const noexcept;               // 仅 tcp 有意义
    [[nodiscard]] const std::error_code& address_error() const noexcept;  // bind 失败原因

    [[nodiscard]] std::size_t connected_sessions() const;
    [[nodiscard]] bool is_ready(std::uint64_t session_id) const;

    [[nodiscard]] bool register_method(std::uint16_t method, Handler handler);
    [[nodiscard]] bool register_notification(std::uint16_t method, Handler handler);

    [[nodiscard]] bool request(std::uint64_t session_id, std::uint16_t method,
                               std::vector<std::byte> payload, Response_handler on_response,
                               std::chrono::milliseconds timeout = std::chrono::seconds(3));
    [[nodiscard]] bool notify(std::uint64_t session_id, std::uint16_t method,
                              std::vector<std::byte> payload);

    [[nodiscard]] std::size_t broadcast_notify(std::uint16_t method,
                                               std::vector<std::byte> payload);
    [[nodiscard]] std::size_t broadcast_request(
        std::uint16_t method, std::vector<std::byte> payload, Broadcast_handler on_response,
        std::chrono::milliseconds timeout = std::chrono::seconds(3));

    void set_on_connected(std::function<void(const Session_ptr&)> handler);
    void set_on_disconnected(std::function<void(const Session_ptr&)> handler);
};
```

### 4.2 Client

```cpp
class Client : public std::enable_shared_from_this<Client>
{
public:
    using Response_handler = rpc::Peer::Response_handler;

    Client(Event_loop& loop, std::string address);

    void connect_async();
    void disconnect();
    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] bool is_ready() const;          // 握手完成
    [[nodiscard]] std::uint64_t session_id() const;

    [[nodiscard]] bool register_method(std::uint16_t method, Handler handler);
    [[nodiscard]] bool register_notification(std::uint16_t method, Handler handler);

    [[nodiscard]] bool request(std::uint16_t method, std::vector<std::byte> payload,
                               Response_handler on_response,
                               std::chrono::milliseconds timeout = std::chrono::seconds(3));
    [[nodiscard]] bool notify(std::uint16_t method, std::vector<std::byte> payload);

    void set_on_connected(std::function<void()> handler);
    void set_on_disconnected(std::function<void()> handler);
    void set_error_handler(std::function<void(const std::error_code&)> handler);
};
```

### 4.3 使用示例

```cpp
// 服务端：一份代码，两条地址
auto server = std::make_shared<serve::Server>(server_loop, "ipc://valley_demo");
server->register_method(1, echo);
server->start();

// 客户端
auto client = std::make_shared<serve::Client>(client_loop, "tcp://127.0.0.1:9000");
client->request(1, payload, [](serve::rpc::Status status, std::vector<std::byte> body) {
    // ...
});
client->connect_async();
```

## 5. 内部结构

```
serve::Server / serve::Client          ← 公开门面：地址解析 + 传输选择
        │
        ├── Ipc_backend  →  rpc::Ipc_server   + ipc::Ipc_server/Ipc_client
        └── Tcp_backend  →  rpc::Tcp_server + rpc::Tcp_client
```

### 5.1 方案 A：门面 + 适配器（先落地，约 2 人天）

```cpp
class Server_backend
{
public:
    virtual ~Server_backend() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool register_method(std::uint16_t, rpc::Handler) = 0;
    // request / notify / broadcast / 回调设置 …
};

class Ipc_server_backend final : public Server_backend { rpc::Ipc_server impl_; };
class Tcp_server_backend final : public Server_backend { rpc::Tcp_server impl_; };
```

- `serve::Server` 构造：解析地址 → `unique_ptr<Server_backend>` 指向具体实现；
- 所有方法只做转发；虚调用只在控制面，不进入每消息热路径；
- 优点：改动小、不动现有 RPC 核心；缺点：两套 backend 各自维护会话表（重复仍在 `rpc::Ipc_server` / `rpc::Tcp_server` 里）。

### 5.2 方案 B：把传输下沉到 acceptor（推荐目标态，+2 人天）

与现在 `Session_base` 的思路一致，继续抽出两个接口：

```cpp
class Acceptor
{
public:
    virtual ~Acceptor() = default;
    virtual bool open(const Address&, std::error_code& ec) = 0;
    virtual void start_accept(std::function<void(Session_base::Ptr)>) = 0;
    virtual void close() = 0;
    virtual std::string local_address() const = 0;   // tcp 回填端口
};

class Connector
{
public:
    virtual ~Connector() = default;
    virtual void connect_async(std::function<void(Session_base::Ptr, std::error_code)>) = 0;
    virtual void close() = 0;
};
```

- `rpc::Ipc_server` 改为持有 `shared_ptr<Acceptor>`，不再依赖 `Ipc_server`；
- `rpc::Ipc_client` 改为持有 `shared_ptr<Connector>`；
- 删掉 `rpc::Tcp_server` 与 `rpc::Ipc_server` 的重复会话表逻辑；
- `serve::Server` 只是「解析地址 → 造 Acceptor → 造 rpc::Ipc_server」的薄壳；
- 未来加 TLS / WebSocket 只需再实现 `Acceptor` / `Connector`。

### 5.3 推荐路径

先做 A（对外接口稳定、可测试），再择机做 B（消除重复）。A 阶段就能把
`tcp_rpc_test` 与 `rpc_test` 合并成「同一套用例、两条地址」的形式。

## 6. 错误处理与边界

| 场景 | 行为 |
|---|---|
| 地址语法错误（scheme、缺端口、非法 IP） | 构造函数抛 `std::invalid_argument`；`Address::parse` 可提前 `optional` 校验 |
| bind 失败（端口占用、路径无权限） | `start()` 返回 `false`，原因在 `address_error()` |
| 客户端连接到不存在的 ipc / tcp | `set_error_handler` 收到 `std::error_code`，`request` 回 `Status::UNAVAILABLE` |
| 跨机器使用 `ipc://` | 不支持，IPC 只在同机；文档明确 |
| Windows 上的 `ipc:///tmp/x` | 非法（Windows 只认管道名） |
| 服务端 `tcp://0.0.0.0:0` | 允许；`address()` 回填实际端口 |
| 客户端地址含 `0.0.0.0` | 拒绝（`std::invalid_argument`） |

## 7. 兼容与迁移

- 现有 `rpc::Ipc_server` / `rpc::Ipc_client` / `rpc::Tcp_*` 保留，作为低级 API；
- 新代码推荐 `Server` / `Client`；旧代码不需要改；
- `valley/serve/serve.h` 追加 `#include "serve/message.h"`；
- 文档：`serve_api.md` 新增门面章节；现有 RPC 章节标注「低级 API，通常直接用 `Server` / `Client`」。

## 8. 测试计划

| 测试 | 内容 |
|---|---|
| `address_test` | 解析/规范化/非法地址/端口回填；Windows 与 POSIX 分支都要覆盖 |
| `address_test` | 解析/规范化/非法地址/端口回填；Windows 与 POSIX 分支 |
| `facade_test` | 同一段代码跑 `ipc://` 与 `tcp://127.0.0.1:0`：握手、echo、通知 |
| `xproc_test` 扩展 | 子进程用 `tcp://` 与 `ipc://` 各跑一次 |
| 基准 | `ipc_latency_bench --address ipc://x` / `--address tcp://h:p`，复用现有统计 |

## 9. 工作量

| 阶段 | 内容 | 预估 |
|---|---|---|
| A1 | `Address` 解析 + `address_test` | 0.5 天 |
| A2 | `serve::Server` / `serve::Client` 门面 + backend 适配 | 1 天 |
| A3 | 双传输测试 + 跨进程测试改造 | 0.5 天 |
| B | 抽 `Acceptor`/`Connector`，消除 rpc::Ipc_server 与 Tcp_server 的重复 | 2 天（可选） |

只做 A：约 2 人天，对外接口立刻可用；做 A+B：约 4 人天，内部只剩一套 RPC 核心。

## 10. 风险

- **Windows/POSIX 的 ipc 语义差异**：Windows 是「管道名」，POSIX 是「文件路径」；
  统一成 `ipc://<name>` 会牺牲 POSIX 绝对路径的表达力，因此保留 `ipc:///abs/path` 形态，
  并在 Windows 上明确拒绝。
- **端口回填**：服务端 `tcp://0.0.0.0:0` 必须能拿到实际端口，`address()` 是唯一的来源，
  测试和上层日志都依赖它。
- **异常 vs 错误码**：地址错误用异常（配置错误，早失败），运行期错误用 `error_code`
  与 `Status`，两者不要混用。
- **重复代码**：方案 A 不解决 `rpc::Ipc_server` / `rpc::Tcp_server` 的会话表重复，
  后续改动要记得两边同步；B 阶段再收敛。
