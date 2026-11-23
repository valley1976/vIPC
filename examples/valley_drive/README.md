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

先把对应平台的 SDK 解压/放到仓库的 `dist/` 下，例如：

```text
dist/valley-1.0.0-windows-x86_64/
dist/valley-1.0.0-linux-x86_64/
```

Windows：

```powershell
$env:VALLEY_CMAKE_PREFIX_PATH = 'C:\path\to\conan\generators'
.\examples\valley_drive\build.ps1
```

Linux：

```bash
export VALLEY_CMAKE_PREFIX_PATH=/path/to/conan/generators
./examples/valley_drive/build.sh
```

`data` / `exec` 的 CMake package 还会查找 `nlohmann_json`、`mcap`、`lz4`、
`zstd` 等依赖，因此通常需要把 Conan 的 generators 目录通过
`VALLEY_CMAKE_PREFIX_PATH` 传进来；也可以直接设置
`VALLEY_CONAN_TOOLCHAIN=<.../conan_toolchain.cmake>`。asio 由仓库内的 shim
处理，不需要额外配置。

默认输出：

```text
build/valley_drive/Release/valley_drive.exe   # Windows VS generator
build/valley_drive/valley_drive               # Linux / 单配置 generator
```

`build.ps1` / `build.sh` 会自动查找 `VALLEY_SDK`、`dist/`、`sdk/` 和
`out/*_Release`；构建目录可用 `VALLEY_BUILD_DIR` 覆盖。

## 运行

运行脚本会自动查找：

```text
VALLEY_SDK
仓库 dist/valley-*-<platform>
仓库根 package 布局（bin/supervisor.exe）
旧 out/*_Release
```

并优先从 `build/valley_drive_win/Release`、`build/valley_drive` 等目录找
`valley_drive.exe`。脚本不会修改 SDK 目录。

### Windows

```powershell
powershell -ExecutionPolicy Bypass -File `
  examples\valley_drive\run_demo.ps1 `
  -Duration 30
```

显式指定 SDK 和示例可执行文件：

```powershell
powershell -ExecutionPolicy Bypass -File `
  examples\valley_drive\run_demo.ps1 `
  -ReleaseDir dist\valley-1.0.0-windows-x86_64 `
  -ExampleExe build\valley_drive\Release\valley_drive.exe `
  -Duration 30
```

默认启动 monitor 窗口；CI 使用：

```powershell
-Headless
```

### Linux

```bash
examples/valley_drive/run_demo.sh
```

第一个参数是运行目录，第二个是时长；使用默认的 `dist/` SDK 时第一个参数留空：

```bash
examples/valley_drive/run_demo.sh "" 30
examples/valley_drive/run_demo.sh dist/valley-1.0.0-linux-x86_64 30
```

Linux 在交互式终端中默认启动 monitor；无 TTY 或 CI 默认只输出一次快照。
强制行为：

```bash
MONITOR=1 examples/valley_drive/run_demo.sh "" 30
MONITOR=0 examples/valley_drive/run_demo.sh "" 30
```

## 手工启动

```bash
# 0. 示例可执行文件（由上面的构建步骤生成）
DRIVE=./build/valley_drive/valley_drive

# 1. supervisor
./dist/valley-1.0.0-linux-x86_64/bin/supervisor \
  --config examples/valley_drive/topology.json

# 2. subscriber
$DRIVE subscriber \
  --application valley-drive \
  --duration 30

# 3. recorder
$DRIVE recorder \
  --application valley-drive \
  --duration 30 --trigger-after 5 --storage out/valley_drive

# 4. publisher
$DRIVE publisher \
  --application valley-drive \
  --duration 30

# 5. monitor
./dist/valley-1.0.0-linux-x86_64/bin/monitor \
  --application valley-drive
```

`valley_drive` 不再手写 `--address`；publisher / subscriber / recorder 都只传
`--application`，由 application 生成与 supervisor 相同的 endpoint。运行脚本会
从 `topology.json` 读取 `application`，因此改拓扑后无需再改地址。

对应 endpoint 为（由 application 生成）：

Windows：

```text
ipc://vIPC-CHN-valley-drive
```

Linux：

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

Supervisor 和 `valley_drive` 的 endpoint 完全由：

```text
application = valley-drive
```

生成，不需要另外指定 `--address`。实现上使用
`serve::Address::make_ipc_address(application, socket_dir)`，与 supervisor 的
寻址规则一致。修改 application 时同时改 `topology.json`；`run_demo` 会自动
读取它并传给 publisher / subscriber / recorder / monitor。

POSIX 下如果需要改变默认的 `/tmp`，supervisor 和 `valley_drive` 都要传相同的
`--socket-dir`。

## 输出目录

运行脚本把 topology 复制到本次运行目录，而不是 SDK 目录：

```text
<current-directory>/valley_drive_<timestamp>/
  conf/topology.json
  supervisor.out/err
  subscriber.out/err
  recorder.out/err
  publisher.out/err
  valley-drive_sensor_*.mcap
  valley-drive_sensor_trigger_*_manifest.json
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

## 说明

- 示例不依赖旧版 `valley_base` / `valley_conf`。
- 只使用当前安装包导出的：

```text
valley::data
valley::exec
valley::ipc
```

- `Synchronized<T>` 可用于 `topology.json` 中的 `shared` 区域。
