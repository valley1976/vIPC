# Windows Release 安装与使用

测试程序位于构建树：

```text
build\Windows_x86_64_Release\bin\
```

`out\Windows_x86_64_Release\bin\` 只包含安装后的应用和动态库。

## 1. 安装位置

Windows Release 产物统一安装在：

```text
out\Windows_x86_64_Release\
```

目录结构：

```text
out\Windows_x86_64_Release\
  bin\
    supervisor.exe
    monitor.exe
    multi_topic_demo.exe
    recording_demo.exe
    recorder_demo.exe
    broker_demo.exe
    supervisor_demo.exe
    cyclic_task_demo.exe
    cyclic_latency_demo.exe
    channel_demo.exe
    channel_echo_bench.exe
    channel_latency_bench.exe
    data_test.exe
    ...

  lib\
    data.lib
    exec.lib
    ipc.lib
    serve.lib
    storage_mcap.lib
    storage_protobuf.lib
    ftxui-*.lib
    ui.lib
    cmake\
      data\
      exec\
      ipc\
      serve\
      storage_mcap\
      valley\

  include\
    data\
    exec\
    ipc\
    serve\
    storage\
```

## 2. 安装命令

只安装 base：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target base
```

安装工具和 monitor：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target tools
```

安装 data：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target data
```

安装 storage：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target storage
```

完整安装：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target all `
  -RunTests
```

注意：`all` 默认包含 `base`、`tools`、`exec`、`ipc`；`data` 和
`storage` 需要单独执行。

---

## 3. 运行 Supervisor

准备拓扑：

```json
{
  "application": "valley-demo",
  "channel": [{
    "group": "/sensor",
    "subscriber": ["realtime", "recorder"],
    "topic": [{
      "name": "/sensor/imu",
      "subscriber": ["realtime", "recorder"],
      "ring_size": 512,
      "data_size": 64
    }]
  }]
}
```

启动：

```powershell
.\out\Windows_x86_64_Release\bin\supervisor.exe `
  --config topology.json
```

带 monitor：

```powershell
.\out\Windows_x86_64_Release\bin\supervisor.exe `
  --config topology.json `
  --monitor
```

Supervisor 会创建：

```text
dsh-chan-<application>-<pid>
dsh-metrics-<application>
```

后者包含：

```text
recorder_metrics
channel_registry
```

---

## 4. 运行 Monitor

地址：

```text
ipc://vIPC-CHN-<application>
```

启动 UI：

```powershell
.\out\Windows_x86_64_Release\bin\monitor.exe `
  --address ipc://vIPC-CHN-valley-demo
```

一次性采样：

```powershell
.\out\Windows_x86_64_Release\bin\monitor.exe `
  --address ipc://vIPC-CHN-valley-demo `
  --once
```

输出示例：

```text
proc pid=30660 state=UP uptime=5s
topic /sensor/imu published=375 rate=0.0/s
subscriber /sensor/imu/realtime pid=25764 dropped=0 lag=1
recorder multi-topic/multi-topic mode=sync written=423 dropped=0
         unwritten=0 queue=0 segments=1 p95=219us
         pre_roll=0 late=0 pub_fail=0
```

Monitor 主路径直接读取：

```text
dsh-metrics-<application>
  -> channel_registry
  -> channel shm
  -> subscriber cursor/dropped
  -> subscriber heartbeat/pid
  -> recorder_metrics
```

`STATS_METHOD` 只在 registry 不可用时 fallback。

---

## 5. 运行 Demo

### multi_topic_demo

```powershell
powershell -ExecutionPolicy Bypass -File `
  valley\data\examples\multi_topic_demo\run_demo.ps1 `
  -ReleaseDir out\Windows_x86_64_Release `
  -Duration 70
```

默认会启动 monitor 窗口。

仅 CI 使用：

```powershell
-Headless
```

`-Headless` 会禁止 monitor。

### recording_demo

```powershell
.\out\Windows_x86_64_Release\bin\recording_demo.exe `
  --config valley\data\examples\recording_demo\recording_config.json `
  --out out\recording_demo_run
```

该 demo 是 data 录制演示，不启动 Supervisor。

### exec / channel demo

```powershell
.\out\Windows_x86_64_Release\bin\cyclic_task_demo.exe
.\out\Windows_x86_64_Release\bin\channel_demo.exe
```

---

## 6. 在 CMake 项目中使用

```cmake
cmake_minimum_required(VERSION 3.20)
project(app LANGUAGES CXX)

set(CMAKE_PREFIX_PATH
    "D:/projects/DSH_valley/out/Windows_x86_64_Release"
    CACHE STRING "")

find_package(data REQUIRED)
find_package(exec REQUIRED)
find_package(ipc REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE
    valley::data
    valley::exec
    valley::ipc)
```

如果只需要 data 和存储：

```cmake
find_package(data REQUIRED)
find_package(storage_mcap REQUIRED)

target_link_libraries(app PRIVATE
    valley::data
    valley::storage_mcap)
```

Protobuf 辅助库单独使用：

```cmake
find_package(storage_protobuf REQUIRED)
target_link_libraries(app PRIVATE valley::storage_protobuf)
```
```

安装包导出的 target 仍使用 `valley::` CMake namespace：

```text
valley::data
valley::exec
valley::ipc
valley::serve
valley::ipc
valley::storage_mcap
```

---

## 7. C++ API

### Recorder

```cpp
#include "data/recorder.h"
#include "data/recorder_config.h"

data::Recorder_config config;
config.name = "R0";

data::Bundle_config bundle;
bundle.name = "B0";
bundle.topics = {"/sensor/imu"};
config.bundles = {bundle};

data::Recorder_options options;
options.storage_directory = ".";
options.metrics_shm_name = "dsh-metrics-valley-demo";

auto recorder = data::Recorder::create(config, options);
```

### Trigger

```cpp
auto trigger = data::Trigger::make_unique(
    "event", "planner", now_ms, 5, 10);
trigger->topic = {"/sensor/imu"};
recorder.record(std::move(trigger));
```

### Channel

```cpp
#include "ipc/channel.h"

ipc::Channel channel{ipc::Channel::Config{512, 2}};
```

---

## 8. 共享内存命名

```text
topic channel:   dsh-chan-<application>-<pid>
metrics:         dsh-metrics-<application>
  regions:       recorder_metrics
                 channel_registry
```

Monitor 从 address 推导 metrics 名：

```text
ipc://vIPC-CHN-<application>
  -> dsh-metrics-<application>
```

---

## 9. 兼容性

当前 Channel 共享内存格式：

```text
FORMAT_VERSION 7
```

包含：

```text
Header
Subscriber_state[]
Subscriber_metrics[]
Slot[]
Chunk pool
```

旧版本 shm 不兼容，需要重启：

```text
Supervisor
Broker
Recorder
Monitor
```

---

## 10. 验证

```powershell
.\out\Windows_x86_64_Release\bin\supervisor_test.exe
.\out\Windows_x86_64_Release\bin\channel_config_test.exe
.\out\Windows_x86_64_Release\bin\broker_test.exe
.\out\Windows_x86_64_Release\bin\monitor_provider_test.exe
.\out\Windows_x86_64_Release\bin\data_test.exe
```

或使用：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 `
  -BuildType Release `
  -Target tools `
  -RunTests
```
