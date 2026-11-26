# Benchmark

四个 benchmark 都只使用 SDK 的公开头（`include/` 下）和 `find_package` 产物，
不引用核心实现源码。

## 构建与运行

```bash
cd benchmarks
./build.sh
```

Windows：

```powershell
cd benchmarks
.\build.ps1
```

两个脚本都会编译全部可用的 benchmark，然后依次运行。也可以手动运行：

```bash
./build/channel_bench
./build/event_bench
./build/synchronized_bench
./build/serve_bench
```

构建脚本会依次查找 `VALLEY_SDK`、仓库 `dist/valley-*-<platform>`、
`仓库 sdk/` 和 `out/*_Release`。`serve_bench` 需要 SDK 提供 `serve`
package；没有时会跳过编译和运行。

## channel_bench

测 `ipc::Channel` 的 publish + 每个 subscriber 各 read 一次的吞吐，支持
scalar 模式和 chunk（payload）模式。

| 参数 | 默认 | 说明 |
|---|---:|---|
| `--samples N` | 200000 | 计时迭代数 |
| `--warmup N` | 1000 | 预热迭代数 |
| `--ring-size N` | 1024 | ring 容量 |
| `--subscribers N` | 1 | subscriber 数，1..63 |
| `--payload N` | 0 | 0 = scalar；>0 = chunk 模式，每轮写 N 字节 |

输出 `ns/iter`、`Miter/s`、`Mvalues/s`。scalar 模式会校验 checksum，
payload 模式会校验每个 subscriber 读到的第一个字节。

```bash
./build/channel_bench --subscribers 4 --ring-size 4096
./build/channel_bench --payload 256 --samples 100000
```

## event_bench

测 `ipc::Event` + `ipc::Notification` 的三条路径：

1. `emit-only`：只 emit，不等待，看单位 emit 成本和吞吐；
2. `event/self`：同线程 `emit()` 后立即 `wait()` + `poll()`，看 API 路径成本；
3. `event/cross`：一个线程 emit，另一个线程 wait，用请求/确认握手逐个测量，
   看包含线程唤醒的延迟。

| 参数 | 默认 | 说明 |
|---|---:|---|
| `--samples N` | 20000 | 延迟样本数 |
| `--emit-samples N` | 2000000 | emit-only 迭代数 |
| `--warmup N` | 2000 | 预热次数 |
| `--no-cross` | 关闭 | 跳过跨线程阶段 |

```bash
./build/event_bench
./build/event_bench --no-cross
```

## synchronized_bench

测 `ipc::Synchronized<Counter>` 的锁开销：

1. 单线程 `apply()`：uncontended lock/unlock；
2. 单线程 `try_apply()`：非阻塞路径，同时统计失败次数；
3. 多线程 `apply()`：多个线程抢同一把锁，统计聚合吞吐。

| 参数 | 默认 | 说明 |
|---|---:|---|
| `--samples N` | 200000 | 单线程迭代数 |
| `--warmup N` | 5000 | 预热迭代数 |
| `--threads N` | min(hw, 4) | 竞争线程数 |
| `--contention-samples N` | 50000 | 每个线程的竞争迭代数 |
| `--no-contention` | 关闭 | 跳过竞争阶段 |

最后会校验 `Counter::value`，确认没有丢失更新。

## serve_bench

用 `serve::Server` / `serve::Client` 做串行 RPC ping-pong：服务端 method 1
原样回显 payload，客户端收到响应后立即发下一发。统计一次 request-response
的往返时延。

| 参数 | 默认 | 说明 |
|---|---:|---|
| `--ipc` / `--tcp` | IPC | 传输选择 |
| `--same-loop` | 关闭 | server/client 共用一个 `Event_loop` |
| `--sweep` | 关闭 | 依次测 16 / 64 / 256 / 1024 / 4096 字节 payload |
| `--samples N` | 20000 | 计时往返数 |
| `--warmup N` | 2000 | 预热往返数 |
| `--payload N` | 16 | 请求 payload 字节数 |

默认两个 `Event_loop`、两个线程，模拟对端进程；`--same-loop` 用单线程，
用来隔离线程唤醒成本。

```bash
./build/serve_bench
./build/serve_bench --tcp
./build/serve_bench --same-loop
./build/serve_bench --sweep
```

### 关于 asio

`serve` 的公开头不包含 asio，asio 只在 `serve` 的 DLL/静态库实现内部使用；
但 SDK 里的 `serveConfig.cmake` 仍会 `find_dependency(asio)`。本目录的
`cmake/asio/asioConfig.cmake` 是一个最小 shim：SDK 没有附带 asio CMake
package 时，它只提供空的 `asio::asio` 目标，让 `find_package(serve)` 能通过。
如果确实需要编译 `serve` 内部头，用：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$VALLEY_SDK" \
      -DVALLEY_ASIO_INCLUDE=/path/to/asio/include
```

## 注意事项

- 使用 Release 构建；Debug 数字没有参考价值；
- 结果受 CPU 频率、电源策略、后台负载影响，比较时应在同一台机器、同一状态下运行；
- `serve_bench` 的延迟包含线程调度，跨 loop 时尤其明显；`--same-loop` 用于看库本身的开销；
- Windows 命名管道 / TCP 首次连接会有额外初始化成本，已被 warmup 排除。
