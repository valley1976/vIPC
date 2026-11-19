# Linux Release 安装与使用

## 1. 安装位置

```text
out/Linux_Release/
  bin/
  lib/
  include/
  lib/cmake/
  lib/systemd/system/
```

安装命令：

```bash
./scripts/build.sh -p all
./scripts/build.sh -p data
./scripts/build.sh -p storage
```

`all` 包含 runtime、tools、exec、ipc；`data` 和 `storage` 单独安装。

## 2. Supervisor

```bash
./out/Linux_Release/bin/supervisor --config topology.json
```

带 monitor：

```bash
./out/Linux_Release/bin/supervisor --config topology.json --monitor
```

Linux IPC 地址示例：

```text
ipc:///tmp/dsh-channel-<application>.sock
```

## 3. Monitor

```bash
./out/Linux_Release/bin/valley_monitor \
  --address ipc:///tmp/dsh-channel-valley-demo.sock
```

一次性输出：

```bash
./out/Linux_Release/bin/valley_monitor \
  --address ipc:///tmp/dsh-channel-valley-demo.sock \
  --once
```

Monitor 直接读取：

```text
dsh-metrics-<application>/channel_registry
  -> channel shm
  -> subscriber cursor/dropped/metrics
  -> recorder_metrics
```

`STATS_METHOD` 仅作为 fallback。

## 4. Demo

```bash
valley/data/examples/multi_topic_demo/run_demo.sh \
  out/Linux_Release 70
```

无界面环境也使用同一个脚本；Linux 脚本不启动 FTXUI monitor。

`recording_demo`：

```bash
./out/Linux_Release/bin/recording_demo \
  --config valley/data/examples/recording_demo/recording_config.json \
  --out out/recording_demo_run
```

## 5. CMake

```cmake
set(CMAKE_PREFIX_PATH
    "/mnt/d/projects/DSH_valley/out/Linux_Release"
    CACHE STRING "")

find_package(data REQUIRED)
find_package(exec REQUIRED)
find_package(ipc REQUIRED)
find_package(shm REQUIRED)

target_link_libraries(app PRIVATE
    valley::data
    valley::exec
    valley::ipc
    valley::shm)
```

导出的 CMake target 仍使用 `valley::` namespace：

```text
valley::data
valley::exec
valley::ipc
valley::serve
valley::shm
valley::storage_mcap
```

## 6. systemd

安装包包含：

```text
out/Linux_Release/lib/systemd/system/supervisor@.service
out/Linux_Release/lib/systemd/system/dsh-publisher@.service
out/Linux_Release/lib/systemd/system/dsh-subscriber@.service
```

示例：

```bash
sudo cp out/Linux_Release/lib/systemd/system/supervisor@.service \
  /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now supervisor@valley-demo
```

具体 ExecStart / 配置文件由部署方调整。

## 7. 测试

```bash
./out/Linux_Release/bin/data_test
./out/Linux_Release/bin/mcap_writer_test
./out/Linux_Release/bin/supervisor_test
./out/Linux_Release/bin/broker_test
./out/Linux_Release/bin/monitor_provider_test
```

或：

```bash
./scripts/build.sh -p data --test
./scripts/build.sh -p storage --test
./scripts/build.sh -p tools --test
```

## 8. 兼容性

Channel 共享内存格式：

```text
FORMAT_VERSION 7
```

需要一起升级：

```text
supervisor
broker
recorder
valley_monitor
```

旧共享内存需要重新创建。
