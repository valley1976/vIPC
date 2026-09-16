// rpc.h —— valley_rpc 的**纯 C++ 公开接口**（推荐入口）
//
// 这个头文件的唯一约束：**只依赖 C++17 标准库**。
// 里面不会出现 kj:: / capnp:: 任何类型，也不会（直接或间接）include capnp / KJ 的头。
// 这条约束不是写在文档里让人自觉遵守，而是由构建系统强制执行的：
// CMake 里的 rpc_public_header_guard 目标只 include 本头、且不链接/不包含
// CapnProto 的 include 目录 —— 一旦这里泄漏了 capnp/KJ 类型，那个目标立刻编译失败。
//
// 两层接口的关系：
//   * rpc.h（本文件）：面向使用者的门面。地址是 std::string，载荷是 std::vector<uint8_t>，
//     回调是 std::function，错误是异常/状态码。业务代码只需要 include 这一个头。
//   * ez-rpc.h：底层实现，暴露 capnp/KJ 类型（kj::Own / kj::Maybe / Capability::Client /
//     ReaderOptions ...）。需要 capnp 原生能力（自定义 schema、typed capability、
//     细粒度控制）时再用它。详见 docs/api.md §14。
//
// 线程模型与事件循环（必须先读）：
//   * **事件循环是显式的**：先建 `valley_rpc::EventLoop`，再把它交给 Server/Client。
//     和底层 `capnp::EventLoop` 一样，循环不是全局单例、也不会被藏进对象里；
//     谁创建、谁运行、谁负责它的生命周期，都由调用方决定。
//   * `EventLoop` 属于**创建它的那个线程**，绑定到它的 Server/Client 也必须在同一个
//     线程上使用（KJ 的硬性约束）。一个线程同时只能有一个 `EventLoop`
//     （违反会抛 Error，而不是让 KJ abort 进程）。
//   * 跨线程只有两个入口：`EventLoop::stop()`（结束别的线程上的 `run()`）和
//     `EventLoop::post()`（往别的线程的循环上投任务）；其余全部限定在属主线程。
//   * 循环自带任务队列与定时器（`post` / `setTimeout` / `setInterval`，见 api.md §14.9）：
//     它们的回调和对端回调一样在循环线程上执行、一样不能阻塞。
//   * **同一个 EventLoop 可以挂多个 Client/Server**（"一线程多节点"）—— 这正是显式循环
//     的好处：一个线程里跑多个连接不需要多个循环，也不会互相"抢"循环。
//   * 谁来驱动循环：
//       - 服务端进程：`loop.run()`（阻塞到 `stop()`）；
//       - 客户端：阻塞调用（`call`/`ping`/…）自己会推进循环，空闲时用 `EventLoop::pump()`
//         收对端推来的事件；
//       - 同进程里既当服务端又当客户端：给服务端单独一个线程 + 一个 EventLoop
//         （见 docs/api.md §14.5）。
//   * **所有用户回调都在所属 EventLoop 的线程上执行**，且**回调里不能再做阻塞调用**
//     （KJ 禁止在事件回调里 wait()）：门面会检测这种误用并抛 Error，要发起的调用请用
//     `callAsync()` / `pushEventAsync()` / `Peer` 的异步接口。
//   * 阻塞接口的 timeout = 0 表示"用 Options::callTimeout"；而 Options::callTimeout = 0
//     表示"不限时"（对端不回复就会一直等）。
//
// 生命周期：**EventLoop 必须比绑定到它的 Server/Client 活得久**（与底层 ez-rpc.h 同一条
// 规则：KJ 的端口/计时器由循环持有）。在属主线程上按"先建 loop、后建对象、逆序销毁"写即可。
//
// 载荷：Request::params / Response::result / Event::payload 都是字节（Bytes）。
// 协议字段是 AnyPointer，门面**一律按 capnp 的 Data 写**；读取时额外容忍 Text
// （capnp 里 Data/Text 是不同类型，对端可能用 setAs<Text>() 写）。要和原生 capnp
// 对端互通，请约定用 Data。
//
// 方法名与协议（rpc_service.capnp 的 RpcService）的对应关系：
//   Client::call        -> invoke
//   Client::ping        -> ping
//   Client::pushEvent   -> pushEvent（单条主动推送）
//   Client::sendEvents  -> events（调用方连续调用 + 自动加屏障，见 sendEvents 注释）
// 两个方向完全对称：Server/Client 回调拿到的 Peer 可以对对端发起同样的调用。

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "export.h"

namespace valley {
namespace serve {

// =====================================================================
// 基础类型

using Bytes = std::vector<std::uint8_t>;
using Millis = std::chrono::milliseconds;
using Micros = std::chrono::microseconds;
// 连接标识（服务端每条连接一个，单调递增、从 1 开始）。
// 与底层 capnp::ConnectionId 是同一个东西（都是 uint64），但门面这里不带任何 capnp 类型。
using ConnectionId = std::uint64_t;

// 默认调用超时（Options::callTimeout 的默认值）。
inline constexpr Millis kDefaultCallTimeout{5000};

// 与 rpc_service.capnp 的 Status 一一对应（数值相同，可直接 cast）。
enum class Status : std::uint16_t {
  ok = 0,
  error = 1,
  invalidRequest = 2,
  notFound = 3,
  timeout = 4,
  busy = 5,
  unsupported = 6,
  cancelled = 7,
  unauthorized = 8,
};

// 状态码的可读名字（诊断/日志用；未知值返回 "unknown"）。
const char* statusName(Status status) noexcept;

// 一次请求。载荷统一是字节（协议里的 AnyPointer 按 Data 传，见 docs/api.md §14.4）。
struct Request {
  std::string service;            // 目标服务名（可选，路由用）
  std::string method;             // 方法名（必填；不认识的方法应回 notFound）
  Bytes params;                   // 参数负载（字节）
  std::uint64_t id = 0;           // 请求 ID，原样回填到 Response::id
  std::uint32_t timeoutMs = 0;    // 调用方期望的超时（0 = 用服务端默认）
  Bytes traceId;                  // 链路追踪 ID（可选）
  std::uint64_t timestampMs = 0;  // 请求发出时间（0 = 框架填当前毫秒时间戳）
};

// 一次响应。
struct Response {
  Status status = Status::ok;
  std::string message;            // 人类可读说明 / 错误信息
  Bytes result;                   // 返回负载（字节）
  std::uint64_t id = 0;           // 对应 Request::id
  std::uint64_t serverTimeMs = 0; // 服务端完成时间（毫秒 Unix 时间戳）

  bool ok() const noexcept { return status == Status::ok; }

  static Response makeOk(Bytes result = {}, std::string message = {});
  static Response makeError(Status status, std::string message);
};

// 一条事件。
struct Event {
  std::string topic;              // 主题，如 "sensor/imu"
  Bytes payload;                  // 事件内容（字节）
  std::uint64_t seq = 0;          // 单调递增序号（去重/乱序检测用）
  std::uint64_t timestampMs = 0;  // 事件发生时间（0 = 框架填当前毫秒时间戳）
  std::string source;             // 事件来源节点/进程（可选）
};

// 异步调用的结果：要么对端答复了（error 为空，看 response.status），
// 要么这次调用在**传输层**失败（error 非空：断开、超时、对端没有 bootstrap……）。
struct Outcome {
  enum class Kind : std::uint8_t { none, timeout, transport, protocol };

  Response response;
  std::string error;      // 非空 = 传输层失败，response 无意义
  Kind kind = Kind::none;

  bool delivered() const noexcept { return error.empty(); }  // 对端答复了（哪怕是业务错误）
  bool ok() const noexcept { return error.empty() && response.ok(); }
};

// =====================================================================
// 异常：只有阻塞式接口会抛异常；异步接口一律通过 Outcome 报告失败。

class Error : public std::runtime_error {
public:
  explicit Error(const std::string& what) : std::runtime_error(what) {}
};

// 连接/断开/对端不可达等传输层失败。
class TransportError : public Error {
public:
  explicit TransportError(const std::string& what) : Error(what) {}
};

// 阻塞调用超过给定超时。
class TimeoutError : public Error {
public:
  explicit TimeoutError(const std::string& what) : Error(what) {}
};

// 定时器句柄（可拷贝、可默认构造）。`cancel()` 幂等且线程安全。
// 语义：cancel() 只保证"回调不再执行"，**不**保证立刻从内核定时器里摘除
// （已经挂上的那一次到点后会发现被取消，然后什么都不做）。
class Timer {
public:
  Timer() = default;
  Timer(const Timer&) = default;
  Timer& operator=(const Timer&) = default;
  Timer(Timer&&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;
  ~Timer();

  struct Impl;   // 内部（同 Peer::Impl 的理由：嵌套类型要在 .c++ 里定义）

  // 句柄非空、宿主循环还活着、且还没有被取消 / 还没有触发过（一次性定时器触发后变 false）。
  bool valid() const noexcept;
  // 取消。返回 true = **这次**调用真的取消了；false = 之前已经取消/已经触发/空句柄。
  bool cancel() noexcept;

private:
  friend class EventLoop;
  explicit Timer(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

  std::shared_ptr<Impl> impl_;
};

// =====================================================================
// EventLoop：显式事件循环（和底层 capnp::EventLoop 一个地位）
//
// 门面**不会**把事件循环藏进 Server/Client 里：先建循环，再建对象。
// 这样做的理由和底层一致（见 design.md §3.1）：
//   * 生命周期显式：谁创建、谁销毁、谁保证"循环比对象活得久"一目了然；
//   * 线程亲缘显式：循环属于创建它的线程，绑定它的对象也必须在该线程使用；
//   * 可以共享：一个 EventLoop 挂多个 Client/Server（一线程多节点），
//     不必为每条连接开一个循环；
//   * 不引入全局单例：测试、多实例、嵌入式场景都能各自持有独立的循环。
class EventLoop {
public:
  // 在**当前线程**创建循环；一个线程同时只能有一个（违反抛 Error，不会把进程带走）。
  EventLoop();
  ~EventLoop() noexcept;

  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;
  EventLoop(EventLoop&&) = delete;
  EventLoop& operator=(EventLoop&&) = delete;

  // 阻塞运行事件循环，直到 stop()（服务端进程的典型用法）。
  // 只能在属主线程调用，且只能调用一次。
  void run();
  // 推进事件循环最多 duration：用来在不发请求时收对端推来的事件 / 跑回调 / 跑定时器。
  // 只能在属主线程的"顶层"调用（回调里调用会抛 Error）。
  void pump(Millis duration);
  // 请求停止：幂等，**可在任意线程调用**（用于从别的线程结束 run()）。
  void stop() noexcept;
  bool running() const noexcept;         // run() 正在执行
  bool stopRequested() const noexcept;   // stop() 已被调用
  bool onOwnerThread() const noexcept;   // 当前线程是不是创建它的那个线程

  // ---- 任务与定时器 ----
  //
  // 三个接口的回调都在**本循环的线程**上执行，并且和"对端回调"同等待遇：
  // 里面**不能做阻塞调用**（KJ 禁止在事件回调里 wait()），要发请求就用
  // callAsync()/pushEventAsync()/Peer 的异步接口。回调里抛异常会被兜住并记 ERROR 日志
  // （不会把循环带走）。

  // 把 task 排到本循环上执行（下一次循环轮次）。**任意线程都可以调用**：
  // 从别的线程调用会把循环唤醒（前提是循环正在被驱动：run() / pump() /
  // 正在进行的阻塞调用返回时会顺带把队列跑掉）。
  void post(std::function<void()> task);

  // 一次性定时器：delay 之后执行一次（delay 为 0 表示"下一轮"，与 post 等价）。
  // **只能在属主线程调用**（在回调里调用也可以，例如在自己的 handler 里安排后续动作）。
  Timer setTimeout(Millis delay, std::function<void()> task);

  // 周期定时器：每 period 执行一次，直到 cancel()。周期从**上一次执行结束**开始算
  // （不做追赶，所以实际间隔 ≥ period）。同样只能在属主线程调用。
  // 想在回调里自我取消：见 api.md §14.9 的写法（句柄是值语义，可以捕获）。
  Timer setInterval(Millis period, std::function<void()> task);

  // 内部实现类型：绑定到本循环的 Server/Client 需要它，用户代码不要碰。
  // （放在公开区是语言要求：嵌套类型要在 .c++ 里定义就必须在类外可访问。
  //  这里只有前置声明，用户拿到它什么也做不了。）
  struct Impl;
  Impl& impl() const noexcept;

private:
  std::unique_ptr<Impl> impl_;
};

// =====================================================================
// 回调类型

class Peer;   // 对端句柄（见下）

// 回复一次请求：**必须恰好调用一次**（异步处理时就晚点调）。
// 忘了调用 = 对端永远等不到响应；而且 capnp 会把该 capability 上的后续调用排在它后面，
// 于是整条连接都会卡住 —— 这是本门面里唯一"用户能把自己挂死"的地方。
using Reply = std::function<void(Response)>;
// 异步调用完成回调：**一定恰好被调用一次**。
using Callback = std::function<void(Outcome)>;
// 处理对端发来的 invoke。抛异常会被框架兜住，回 Status::error + 异常文本（不会杀服务端）。
using RequestHandler = std::function<void(const Request&, const Peer&, Reply)>;
// 处理对端发来的 events 流 / pushEvent。返回值就是 pushEvent 的应答状态
// （events 流没有应答，返回值被忽略）。抛异常同样被兜住并回 Status::error。
using EventHandler = std::function<Status(const Event&, const Peer&)>;
// 框架级错误（连接失败、重连失败、回调里抛异常……）。回调内不得抛异常。
using ErrorHandler = std::function<void(std::string)>;
// 已建立的连接断开（Client 用；每条连接一次，初次连接失败走 onError）。
using DisconnectHandler = std::function<void()>;

// =====================================================================
// Peer：对端句柄（双向对等的关键）
//
// 在任意回调里都能拿到一个 Peer，用它向对端发起调用 —— 谁先连上谁并不影响权限。
// Peer 是**值语义**（内部 shared_ptr，拷贝很便宜），可以安全地拷进异步回调里长期持有；
// 所属对象销毁 / 连接断开后再调用，会立刻以 Outcome.error 失败（不崩、不挂）。
//
// **怎么拿到 Peer**（四种入口）：
//   1. 客户端侧（对端 = server）：`client.peer()`；回调参数里给的那个也是它。
//   2. 服务端侧（对端 = 某个 client）：`onRequest` / `onEvent` 的第二个参数，
//      或者 `server.peerFor(id)`（id 来自 `server.connections()` / `server.peers()`）。
//      每条连接一个句柄。
//   3. 服务端想在回调之外主动推事件：`server.peers()` 遍历当前所有连接，
//      或 `server.broadcast(event)` 广播给所有连接 —— 两者都必须在服务端的
//      事件循环线程上调用（也就是回调所在的那个线程；回调里调用也是允许的）。
//   4. 想自己管理连接：把回调参数里的 Peer 拷进自己的注册表（值语义），
//      之后用 `alive()` 清理断开的连接（见 `Peer::connectionId()`）。
//   对端的 bootstrap 只有在对方设置了 `onRequest`/`onEvent` 时才存在；没装时服务端的
//   Peer 依然 valid、alive，但调用会干净地以 Outcome.error 失败。
class Peer {
public:
  Peer() = default;   // 空句柄：valid() == false
  Peer(const Peer&) = default;
  Peer& operator=(const Peer&) = default;
  Peer(Peer&&) noexcept = default;
  Peer& operator=(Peer&&) noexcept = default;
  ~Peer();

  // 内部实现类型的前置声明（用户代码无法构造、也无法使用它；放在这里是语言要求：
  // 嵌套类型要在 .c++ 里定义，就必须在类外可访问）。
  struct Impl;

  bool valid() const noexcept { return impl_ != nullptr; }
  // 对端此刻是否还在（"上次观察"的结论，不作为同步依据）。两侧含义不同：
  //   * 服务端的 Peer 绑在**单条连接**上：连接断开后 `alive()` 变 false，
  //     再调用会拿到"对端连接已断开"的传输层错误；
  //   * 客户端的 Peer 与连接无关（每次调用重新取 capability，重连后依旧可用）：
  //     `alive()` 表示"客户端对象还在"；要问当前连没连上用 `Client::connected()`。
  bool alive() const noexcept;

  // 这个句柄属于哪条连接（服务端侧有意义）：与 `Server::connections()` 返回的 id 对应。
  // 客户端的 Peer 不绑定单条连接（跨重连有效），空句柄也一样，都返回 0。
  ConnectionId connectionId() const noexcept;

  // ---- 异步接口（回调里只能用这些版本，见文件头线程模型）----
  // timeout 为 0 表示用所属 Server/Client 的 Options::callTimeout。
  void call(const Request& request, Callback callback, Millis timeout = Millis{0}) const;
  void call(std::string_view method, Bytes params, Callback callback,
            Millis timeout = Millis{0}) const;
  void pushEvent(const Event& event, Callback callback, Millis timeout = Millis{0}) const;
  // 连续发一批事件（协议里的 events 流）：逐条发、每条等背压窗口，最后再发一次 ping
  // 当屏障 —— 回调被调用时，对端确实已经处理完全部事件。
  void sendEvents(std::vector<Event> events, Callback callback,
                  Millis timeout = Millis{0}) const;

private:
  friend class Client;
  friend class Server;

  explicit Peer(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

  std::shared_ptr<Impl> impl_;
};

// =====================================================================
// ServerOptions / ClientOptions

struct ServerOptions {
  // 监听地址。三种形式（详见 docs/api.md §4）：
  //   "127.0.0.1" / "0.0.0.0" + port   TCP
  //   "unix:/tmp/x.sock"               UDS 文件路径（Windows / Linux）
  //   "unix-abstract:name"             UDS 抽象命名空间（**仅 Linux**）
  std::string bindAddress = "127.0.0.1";
  std::uint16_t port = 0;   // 0 = 让内核选一个空闲端口

  RequestHandler onRequest; // 对端 invoke 本端；不设则本端不装 bootstrap
  EventHandler onEvent;     // 对端用 events / pushEvent 推给本端
  ErrorHandler onError;     // 框架级错误（在本对象的事件循环线程调用）

  Millis callTimeout = kDefaultCallTimeout;  // 本端发起的调用默认超时（0 = 不限时）
};

struct ClientOptions {
  std::string address = "127.0.0.1";  // 目标地址，形式同 ServerOptions::bindAddress
  std::uint16_t port = 0;

  RequestHandler onRequest; // 服务端反向调用本端；不设则对端拿不到本端 bootstrap
  EventHandler onEvent;
  ErrorHandler onError;
  DisconnectHandler onDisconnect;  // 已建立的连接断开时（仅断开的连接）

  bool autoReconnect = false;      // 断开后自动重连（指数退避 ×2，上限 5s）
  Millis reconnectDelay{200};      // 首次重连延迟

  Millis callTimeout = kDefaultCallTimeout;
};

// =====================================================================
// Server：监听方（绑定在调用方给的 EventLoop 上）

// 广播时每条连接的结果（连接 id + 该次推送的结果）
using BroadcastCallback = std::function<void(ConnectionId, Outcome)>;

class Server {
public:
  // 在给定的事件循环上开始监听：绑定失败会**同步**抛 TransportError（消息里带原始错误）。
  // loop 必须比本对象活得久，并且之后所有成员调用都必须在 loop 的属主线程上进行。
  Server(EventLoop& loop, ServerOptions options);
  ~Server() noexcept;

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  // 实际监听的端口（TCP 才有意义；UDS 返回 0）。构造返回时已经确定。
  std::uint16_t port() const noexcept;
  // 构造时给的绑定地址字符串。
  const std::string& bindAddress() const noexcept;
  // 是否还在监听（stop() 之后为 false）。
  bool listening() const noexcept;

  // ---- 连接列表与对端句柄（都只能在 loop 的属主线程调用；**回调里调用是允许的**，
  //      因为它们不会阻塞，只是取快照 / 发起异步推送）----

  // 当前活跃连接的 id（快照）。id 单调递增、从 1 开始，连接断开后不会被复用。
  std::vector<ConnectionId> connections() const;

  // 某条连接的对端句柄；连接不存在或已断开 → 返回**空句柄**（valid() == false）。
  Peer peerFor(ConnectionId id) const;

  // 所有活跃连接的对端句柄（快照）。等价于对 connections() 逐个 peerFor()。
  std::vector<Peer> peers() const;

  // 给当前所有活跃连接推一条事件（best-effort，异步发出去就返回）。
  //   * 返回"尝试推送的连接数"（= 调用瞬间的活跃连接数）；
  //   * 每条连接的结果走 callback（给了 callback 就不会再走 onError）；
  //   * 没给 callback 时，失败的连接会被报到 onError（没注册 onError 则记日志）；
  //   * timeout 为 0 表示用 ServerOptions::callTimeout。
  std::size_t broadcast(const Event& event, BroadcastCallback callback = nullptr,
                        Millis timeout = Millis{0});

  // 停止服务：停止接受新连接 + 关闭现有连接。幂等。
  // **只能在 loop 的属主线程调用**（要跨线程停整个服务，先 `EventLoop::stop()` 结束
  // `run()`，再在属主线程上销毁本对象）。
  void stop();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// =====================================================================
// Client：拨号方（绑定在调用方给的 EventLoop 上）

struct PingResult {
  bool ok = false;                // 心跳成功（传输正常且 Status::ok）
  Status status = Status::error;  // 对端返回的健康状态
  std::uint64_t seq = 0;          // 回显的序号
  Micros rtt{0};                  // 本端测得的往返时间
  std::uint64_t serverTimeMs = 0; // 对端时间（对时用）
  std::uint8_t load = 0;          // 对端负载 0~100（本实现固定 0）
};

class Client {
public:
  // 在给定的事件循环上发起连接。**连接失败不在这里抛**（异步），失败走 onError；
  // 之后 call() 会抛 TransportError。
  // 唯一同步抛的情况是参数本身非法（例如 autoReconnect 用在不可重放的地址形式上）。
  // loop 必须比本对象活得久，且所有成员调用都必须在 loop 的属主线程上。
  // 同一个 loop 上可以挂多个 Client（各自独立连接、互不影响）。
  Client(EventLoop& loop, ClientOptions options);
  ~Client() noexcept;

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  // ---- 阻塞接口：只能在 loop 属主线程的"顶层"调用（不能在自己/对端的回调里）----
  // 会自己推进事件循环直到本次调用完成（所以本线程此时不能同时跑 run()）。
  // timeout 为 0 表示用 Options::callTimeout。
  Response call(const Request& request, Millis timeout = Millis{0});
  Response call(std::string_view method, Bytes params = {}, Millis timeout = Millis{0});
  Status pushEvent(const Event& event, Millis timeout = Millis{0});
  // 一批事件的流式发送。失败抛异常；注意超时可能意味着"前若干条已经发出去了"
  // （屏障 ping 没回来 = 无法确认整批落地）。
  void sendEvents(std::vector<Event> events, Millis timeout = Millis{0});
  PingResult ping(Millis timeout = Millis{0});

  // ---- 异步接口：任何地方都能用（包括回调里），结果走 Callback ----
  void callAsync(const Request& request, Callback callback);
  void pushEventAsync(const Event& event, Callback callback);

  // ---- 连接状态 ----
  // 上一次观察到的连接状态（连接建立/断开、调用成功/失败时更新），不是实时探测。
  bool connected() const noexcept;
  // 主动确认连接可用：内部做一次 ping，成功即 true（会阻塞，最多 timeout）。
  bool waitConnected(Millis timeout);
  // 拿一个对端句柄（等价于传给回调的那个）：异步调用用；连接没建立时回调会以错误失败。
  Peer peer() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace serve
}  // namespace valley
