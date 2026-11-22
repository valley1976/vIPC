# Valley Drive

多进程自动驾驶数据流示例：

```text
supervisor   创建 Channel 共享内存并管理 attach
publisher    生成 6 路传感器数据
subscriber   realtime 订阅并统计
recorder     通过 data::Recorder 录制 MCAP
monitor      读取 supervisor metrics / channel 状态
```

## 构建

先安装 Windows Release 或 Linux Release，然后：

```powershell
cmake -S examples\valley_drive -B build\valley_drive `
  -DCMAKE_PREFIX_PATH=D:/projects/DSH_valley/out/Windows_x86_64_Release `
  -DVALLEY_ASIO_INCLUDE=D:/libs/asio-1.36.0/include
cmake --build build\valley_drive --config Release
```

如果本机依赖由 Conan 管理，额外传入：

```powershell
-DCMAKE_TOOLCHAIN_FILE=<build>/Windows_Release/data/conan/build/generators/conan_toolchain.cmake
```

Linux：

```bash
cmake -S examples/valley_drive -B build/valley_drive \
  -DCMAKE_PREFIX_PATH=$PWD/out/Linux_x86_64_Release
cmake --build build/valley_drive -j
```

## 运行

### Windows

```powershell
powershell -ExecutionPolicy Bypass -File `
  examples\valley_drive\run_demo.ps1 `
  -ReleaseDir out\Windows_x86_64_Release `
  -Duration 30
```

默认启动 monitor 窗口；CI 使用：

```powershell
-Headless
```

### Linux

```bash
examples/valley_drive/run_demo.sh out/Linux_x86_64_Release 30
```

Linux 在交互式终端中默认启动 monitor；无 TTY 或 CI 默认只输出一次快照。
强制行为：

```bash
MONITOR=1 examples/valley_drive/run_demo.sh out/Linux_x86_64_Release 30
MONITOR=0 examples/valley_drive/run_demo.sh out/Linux_x86_64_Release 30
```

## 手工启动

```bash
# 1. supervisor
./out/Linux_x86_64_Release/bin/supervisor \
  --config examples/valley_drive/topology.json

# 2. subscriber
./out/Linux_x86_64_Release/bin/valley_drive subscriber \
  --address ipc:///tmp/vIPC-CHN-valley-drive.sock \
  --duration 30

# 3. recorder
./out/Linux_x86_64_Release/bin/valley_drive recorder \
  --address ipc:///tmp/vIPC-CHN-valley-drive.sock \
  --duration 30 --trigger-after 5 --storage out/valley_drive

# 4. publisher
./out/Linux_x86_64_Release/bin/valley_drive publisher \
  --address ipc:///tmp/vIPC-CHN-valley-drive.sock \
  --duration 30

# 5. monitor
./out/Linux_x86_64_Release/bin/monitor \
  --address ipc:///tmp/vIPC-CHN-valley-drive.sock
```

Windows 地址：

```text
ipc://vIPC-CHN-valley-drive
```

Linux 地址：

```text
ipc:///tmp/vIPC-CHN-valley-drive.sock
```

## 配置

`topology.json`：

```json
{
  "application": "valley-drive",
  "channel": [{
    "group": "/sensor",
    "subscriber": ["realtime", "recorder"],
    "topic": [
      {"name": "/sensor/camera_front", "subscriber": ["realtime", "recorder"]},
      {"name": "/sensor/imu", "subscriber": ["realtime", "recorder"]},
      {"name": "/sensor/lidar", "subscriber": ["realtime", "recorder"]}
    ]
  }],
  "shared": [
    {"name": "param", "size": "512B"}
  ]
}
```

Supervisor endpoint 完全由：

```text
application = valley-drive
```

生成，不需要另外指定。

## 输出目录

运行脚本会把 topology 复制到 Release 的 conf 目录，并让 supervisor 使用该副本：

```text
out/Windows_x86_64_Release/conf/topology.json
out/Linux_x86_64_Release/conf/topology.json
```

## 角色

### publisher

获取 6 个 publisher：

```text
/sensor/camera_front
/sensor/camera_near
/sensor/camera_left
/sensor/camera_right
/sensor/imu
/sensor/lidar
```

通过 `exec::broker::Publisher::acquire_chunk()` 零拷贝发布。

### subscriber

使用 `Broker::acquire_subscriber` 订阅同样 6 个 topic，统计接收数量。

### recorder

使用 `data::Recorder`：

```text
bundle sensor
  topics = 6 个 sensor topic
  max_file_size = 512MB
  write_mode = ASYNC
```

`--trigger-after 5` 秒后触发：

```text
before = 3s
after  = 5s
```

写入 MCAP 和 manifest。

### monitor

直接读取：

```text
dsh-metrics-<application>/channel_registry
channel shm
recorder_metrics
```

显示 Topic、Subscriber、Recorder 状态。

## 输出

```text
<current-directory>/valley_drive_<timestamp>/
  supervisor.out/err
  subscriber.out/err
  recorder.out/err
  publisher.out/err
  valley-drive_sensor_*.mcap
  valley-drive_sensor_trigger_*_manifest.json
```

## 说明

- 示例不依赖旧版 `valley_base` / `valley_conf`。
- 只使用当前安装包导出的：

```text
valley::data
valley::exec
valley::ipc
```

- `Synchronized<T>` 可用于 `topology.json` 中的 `shared` 区域。
