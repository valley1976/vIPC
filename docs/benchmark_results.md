# Benchmark 结果（Windows / MSVC）

> 采集时间：2026-11-23  
> 采集方式：默认参数连续运行 3 次取中位数；“范围”列是 3 次的最小值–最大值。  
> 结果只对下面这台机器和这套构建有效，不能直接当作其他平台的绝对值。

## 1. 测试环境

| 项 | 值 |
|---|---|
| 机器 | TIMI RedmiBook 16 |
| CPU | Intel Core i5-1035G1 @ 1.00 GHz，8 个逻辑处理器 |
| 操作系统 | Windows 10 Pro 26H2，10.0.26300.9974，x64 |
| 电源方案 | 平衡 |
| 编译器 | MSVC 19.29.30159（Visual Studio 2019 16.11） |
| CMake | 3.31.2 |
| Valley SDK | 1.0.0（`D:\projects\DSH_valley\out\Windows_x86_64_Release`） |
| 构建 | Release，x64，NMake Makefiles |

benchmark 源码与参数说明见 [benchmarks/README.md](../benchmarks/README.md)。

## 2. 构建与运行

```powershell
$env:VALLEY_SDK = 'D:\projects\DSH_valley\out\Windows_x86_64_Release'

cd D:\projects\valley-sdk\benchmarks
cmake -S . -B build -G "NMake Makefiles" `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="$env:VALLEY_SDK"
cmake --build build

.\build\channel_bench.exe
.\build\event_bench.exe
.\build\synchronized_bench.exe
.\build\serve_bench.exe
```

也可以直接用 [benchmarks/build.ps1](../benchmarks/build.ps1) 或
[benchmarks/build.sh](../benchmarks/build.sh)，脚本会依次运行全部可用 benchmark。

## 3. 默认场景结果

### 3.1 channel_bench

参数：`ring=1024`、`subscribers=1`、`payload=0`、`samples=200000`、`warmup=1000`。

| 指标 | 中位数 | 范围 |
|---|---:|---:|
| publish + read | 94.8 ns/iter | 93.2–95.1 |
| 迭代吞吐 | 10.55 Miter/s | 10.52–10.73 |

一次迭代包括一次 `publish()` 和一次 subscriber 的 `try_read()`；checksum 校验通过。

### 3.2 event_bench

参数：`samples=20000`、`emit-samples=2000000`、`warmup=2000`。

| 指标 | 中位数 | 范围 |
|---|---:|---:|
| emit-only | 11.5 ns/op，86.81 Mops/s | 11.4–11.7 ns，85.58–87.43 Mops/s |
| event/self mean | 0.52 us | 0.51–0.52 |
| event/self p99 | 0.70 us | 0.70 |
| event/cross mean | 5.42 us | 5.35–5.64 |
| event/cross p99 | 18.80 us | 18.00–20.80 |

`event/self` 是同线程 `emit()` 后立即 `wait(0)`；`event/cross` 是一个线程
emit、另一个线程 wait。跨线程的 p50 受调度影响明显（3 次运行在 2.6–6.5 us
之间波动），因此这里主要看 mean 和 p99。

### 3.3 synchronized_bench

参数：`samples=200000`、`warmup=5000`、竞争阶段 `threads=4`、
`contention-samples=50000/线程`。

| 指标 | 中位数 | 范围 |
|---|---:|---:|
| uncontended `apply` | 705.3 ns/op | 691.2–710.1 |
| uncontended `try_apply` | 705.1 ns/op | 697.6–741.7 |
| 4 线程竞争 `apply` | 6.53 us/op | 5.95–6.94 |
| 竞争聚合吞吐 | 0.15 Mops/s | 0.14–0.17 |

这里的锁是跨进程的 `ipc::Robust_mutex`，在 Windows 上落到命名内核 mutex，
不是 `std::mutex`；因此未竞争也有约 0.7 us，竞争后上升约 9 倍。最后
`Counter` 校验通过，没有丢失更新。

### 3.4 serve_bench

参数：IPC、server/client 各一个 `Event_loop`、`payload=16`、
`samples=20000`、`warmup=2000`。串行 ping-pong：服务端 method 1 原样回显。

| 指标 | 中位数 | 范围 |
|---|---:|---:|
| 往返 mean | 22.70 us | 22.43–24.58 |
| 往返 p50 | 20.00 us | 19.80–20.00 |
| 往返 p99 | 63.20 us | 63.00–75.00 |
| 串行吞吐 | 44.1k rt/s | 40.7k–44.6k |

这个结果和 [serve_usage.md](serve_usage.md) 里“双线程双 loop RPC 约 20–21 us”
的记录同一量级。

## 4. 变体结果

以下每个场景只运行一次，用于观察相对开销，不作为精确基准。

| 场景 | 关键结果 | 说明 |
|---|---|---|
| `channel_bench --subscribers 4` | 250.1 ns/iter，15.99 Mvalues/s | 4 个 subscriber 各读一次 |
| `channel_bench --payload 256` | 104.8 ns/iter，9.55 Miter/s | 256B chunk 模式 |
| `serve_bench --same-loop` | mean 7.48 us，p50 6.70 us，p99 21.20 us，133.7k rt/s | 单 loop/单线程 |
| `serve_bench --tcp` | mean 52.36 us，p50 44.80 us，p99 142.60 us，19.1k rt/s | TCP 回环，双 loop |

## 5. 观察

- **Channel**：单 subscriber 的 publish+read 约 95 ns；4 subscriber 约 250 ns，
  平均每多一个 subscriber 增加约 52 ns。256B chunk 只比 scalar 多约 10 ns，
  这个尺寸下 payload 拷贝不是主要成本。
- **Event**：emit 本身约 11.5 ns，但同线程 `emit+wait` 要约 0.52 us——
  等待原语（OS auto-reset event）占主导；跨线程平均约 5.4 us，p99 到 19 us，
  线程唤醒和调度是主要成本。
- **Synchronized**：跨进程 robust mutex 的未竞争成本约 0.7 us；4 线程竞争时
  约 6.5 us/op，聚合只有约 0.15 Mops/s。这个基准不适合拿来和普通
  `std::mutex` 的几十 ns 做横向比较。
- **Serve**：IPC 双 loop 往返约 22.7 us；单 loop 约 7.5 us，说明约 15 us
  花在两次 loop/线程切换上；TCP 回环约 52 us。RPC 层本身的开销相对较小。
- 这台机器是 1.0 GHz 基准频率的移动 CPU，且使用“平衡”电源方案；插电/高性能
  模式、后台负载和 CPU 频率都会明显改变绝对值。做版本对比时应保持同一台机器、
  同一电源方案和同一构建配置。

## 6. 备注

- `serve_bench` 只使用公开 `serve::Server` / `serve::Client` 门面；SDK 的
  `serveConfig.cmake` 仍会查找 asio，仓库里的
  [asioConfig.cmake](../benchmarks/cmake/asio/asioConfig.cmake) 是满足该依赖的
  最小 shim。
- 本文结果来自当前工作副本；Valley SDK 版本为 1.0.0。
