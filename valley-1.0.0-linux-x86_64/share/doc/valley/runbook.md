# Valley Windows Release 运行说明

Linux Release 安装与使用见 [linux_release_usage.md](linux_release_usage.md)。

本文对应：

```text
out/Windows_x86_64_Release/
```

## 1. 目录

```text
out/Windows_x86_64_Release/
  bin/
    supervisor.exe         控制面进程
    monitor.exe     终端监控
    recorder_demo.exe      MCAP 录制示例
    broker_demo.exe        Broker 客户端示例
    supervisor_demo.exe    Supervisor/Broker 联合示例
  lib/
    exec.lib
    ipc.lib
    serve.lib
    storage_mcap.lib
    data.lib
  include/
    exec/
    ipc/
    serve/
    storage/
    data/
  lib/cmake/
    valley/
    exec/
    ipc/
    serve/
    storage_mcap/
    data/
```

`bin/` 中还会出现测试程序，例如：

```text
supervisor_test.exe
broker_test.exe
data_test.exe
mcap_writer_test.exe
```

这些不是生产进程。

## 2. 前置条件

- Windows 10/11 x64；
- 安装 MSVC 2022 运行库；
- 当前终端可以访问 `out/Windows_x86_64_Release/bin`；
- 若使用 `--monitor`，需要在 TTY 前台运行。

为方便命令，可先进入 Release 目录：

```powershell
cd out\Windows_x86_64_Release
```

## 3. 配置

新建 `topology.json`：

```json
{
  "application": "valley-drive",
  "channel": [{
    "group": "/sensor",
    "subscriber": ["realtime"],
    "topic": [
      {
        "name": "/sensor/imu",
        "subscriber": ["realtime"],
        "ring_size": 384,
        "data_size": 64
      },
      {
        "name": "/sensor/camera_front",
        "subscriber": ["realtime"],
        "ring_size": 256,
        "data_size": 921792
      }
    ]
  }]
}
```

约定：

- `application` 同时用于共享内存和 serve endpoint 命名；
- 同一主机上正在运行的实例，`application` 必须唯一；
- topic 名全局唯一，并直接作为共享内存 region 名；
- `ring_size` 在 topic 上设置；
- `data_size` 为单条 chunk 的 payload 容量，`0` 表示 scalar。

Windows 下 endpoint 为：

```text
ipc://vIPC-CHN-valley-drive
```

Linux 下为：

```text
ipc:///tmp/vIPC-CHN-valley-drive.sock
```

## 4. 启动 Supervisor

```powershell
.\bin\supervisor.exe --config topology.json
```

启动后会创建：

```text
一块 Shared_memory
  ├─ /sensor/imu
  └─ /sensor/camera_front
```

并监听唯一 endpoint：

```text
ipc://vIPC-CHN-valley-drive
```

前台运行时按 `Ctrl+C` 退出。

## 5. 启动 Monitor

另开终端：

```powershell
.\bin\monitor.exe --address ipc://vIPC-CHN-valley-drive
```

无 TTY 或只需要一次采样：

```powershell
.\bin\monitor.exe `
  --address ipc://vIPC-CHN-valley-drive `
  --once
```

输出示例：

```text
proc pid=3312 state=UP uptime=12s
topic /sensor/imu published=1800 rate=30.0/s
```

监控内容：

- Supervisor pid / uptime / shm；
- topic published；
- 采样速率；
- Broker 上报的 subscriber lag / dropped。

## 6. Supervisor 开发模式

```powershell
.\bin\supervisor.exe --config topology.json --monitor
```

行为：

- 只在前台 TTY 启用；
- 自动启动 `monitor`；
- 可用 `MONITOR_EXE` 指定 monitor 路径：

```powershell
$env:MONITOR_EXE = "D:\tools\monitor.exe"
.\bin\supervisor.exe --config topology.json --monitor
```

Service 模式会拒绝 `--monitor`。

## 7. 录制示例

`recorder_demo.exe` 会自行生成配置、启动 Supervisor、启动 Broker、录制 MCAP。

```powershell
cd $env:TEMP
D:\projects\DSH_valley\out\Windows_x86_64_Release\bin\recorder_demo.exe `
  D:\projects\DSH_valley\out\Windows_x86_64_Release\bin\supervisor.exe
```

输出示例：

```text
recording pre-roll...
trigger recorded; recording live...
==> supervisor stats: pid=27296 shm=32768 bytes, topics=1
    /demo/imu published=60 subscriber=1/1
mcap: recorder-demo_20261118_000241_0.mcap
```

产物：

```text
recorder-demo_*.mcap
```

可由 Foxglove / MCAP 工具打开。

## 8. 联合示例

```powershell
.\bin\supervisor_demo.exe .\bin\supervisor.exe
```

演示：

- Supervisor 按配置创建 Channel；
- Broker 单例 attach；
- publisher / subscriber；
- 共享内存读取；
- Broker 停止后 Supervisor 回收 session。

## 9. Broker 示例

`broker_demo.exe` 是进程级示例，需要传入 Supervisor 路径：

```powershell
.\bin\broker_demo.exe .\bin\supervisor.exe
```

它会：

1. 启动临时 Supervisor；
2. 使用一个 Broker 同 session attach 多个 topic；
3. 发布 / 读取 chunk；
4. 查询 Broker subscriber stats。

## 10. 代码接入

安装目录已经包含 CMake package：

```cmake
find_package(exec REQUIRED)
find_package(data REQUIRED)

target_link_libraries(app PRIVATE
  valley::exec
  valley::data)
```

各组件独立安装和查找；不再提供 `valley::runtime` 聚合 target。

数据录制：

```cmake
find_package(data REQUIRED)
target_link_libraries(app PRIVATE valley::data)
```

MCAP：

```cmake
find_package(storage_mcap REQUIRED)
target_link_libraries(app PRIVATE valley::storage_mcap)
```

最小 C++ 使用：

```cpp
#include "exec/broker/broker.h"

auto& broker = exec::broker::Broker::instance();

exec::broker::Broker_options options;
options.client_name = "realtime";
options.addresses = {"ipc://vIPC-CHN-valley-drive"};

if (!broker.start(options))
    return 1;

auto publisher = broker.acquire_publisher("/sensor/imu");
publisher->publish(data, size);
```

## 11. 停止

推荐顺序：

```text
Recorder.stop()
  -> Broker.stop()
  -> Supervisor Ctrl+C / 停止服务
```

不要先停止 Supervisor 再停止 Recorder；Channel 生命周期由 Broker 持有。

## 12. 故障排查

### endpoint 连接失败

确认 `application` 与地址一致：

```text
application = valley-drive
endpoint    = ipc://vIPC-CHN-valley-drive
```

### topic attach 失败

检查：

- topic 名是否与配置完全一致；
- subscriber 名是否在 group/topic 白名单中；
- 是否已经有一个 publisher 占用该 topic。

### 没有 MCAP 文件

检查：

- 当前目录是否可写；
- Trigger 是否收到；
- Recorder 是否在 `stop()` 后关闭文件。

### `--monitor` 没有启动

- 是否在 TTY 前台运行；
- 是否处于 Windows Service 模式；
- `MONITOR_EXE` 是否指向可执行文件；
- `monitor.exe` 是否在 PATH 中。

### 多个 Supervisor 冲突

同一主机上 `application` 必须唯一；否则 endpoint 和共享内存都会冲突。
