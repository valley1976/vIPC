# Valley public API surface

> 目标：每个组件只发布一个主门面、少量值类型和一个明确的高级入口。
> 当前代码尚未对外发布，允许硬切。

## 1. 原则

```text
公开头只出现：
  - 门面类型
  - 值类型
  - 回调/句柄
  - 少量枚举

不出现：
  - wire protocol 布局
  - OS handle
  - 共享内存 slot 原始布局
  - 内部实现类
  - 兼容 alias
```

## 2. 当前公开面

### base

```text
base/export.h
base/static_string.h
base/aligned_allocator.hpp

base/log/log.h
base/log/log_fmt.h
base/log/context.h
base/log/config.h
base/log/severity.h
base/log/record.h
base/log/sink.h
base/log/backend.h
```

`Log_system` 和 sink 实现已内部化到：

```text
valley/base/src/
```

公开后端只保留：

```cpp
base::make_std_sink()
base::make_file_sink()
base::make_rotating_file_sink()
base::make_async_sink()
```

### ipc

```text
ipc/channel.h
ipc/channel_advanced.h   // 可选：恢复 / subscriber 观测
ipc/event.h
ipc/notification.h
ipc/synchronized.h
ipc/synchronized_pool.h
```

`Synchronized_pool::create()` 的返回值是 owner；owner 析构会销毁 READY slot 内的
`Robust_mutex`。`Synchronized_pool::open()` 是非 owner，不销毁共享对象。

### time

```text
time/time.h
```

### shm

```text
shm/shared_memory.hpp
```

### serve

```text
serve/serve.h
serve/address.h
serve/status.h
serve/types.h
serve/event_loop.h
```

低级 TCP/IPC/RPC 实现留在 `serve/src`。

### exec

```text
exec/cyclic_task.hpp
exec/workset.hpp
exec/broker/broker.h
exec/supervisor/supervisor.h
exec/supervisor/config.h
```

`supervisor` wire protocol 已内部化到：

```text
valley/exec/src/supervisor/supervisor_protocol.h
valley/exec/src/supervisor/supervisor_rpc.h
```

公开的 supervisor 统计类型为：

```cpp
exec::supervisor::Supervisor_stats
```

不再是 `Stats_response`。

### data

```text
data/data.h
data/recorder.h
data/trigger.h
data/recorder_config.h
```

### storage

```text
storage/mcap_writer.h
```

## 3. 已完成的收紧

- 删除 `Wake_up` / `make_wake_up` / shared wakeup API；
- Workset 只使用 `ipc::Notification`；
- `Event` 只 emit，`Notification` wait；
- `Notification` 内部状态和 Signals 不公开；
- `Log_system` 不再公开；
- supervisor `Attach/Stats/Report` wire 结构不再公开；
- `Supervisor::stats()` 返回 `Supervisor_stats` 值类型；
- `data` 原始 recorder metrics 布局不再公开；
- 删除 `cyclic_task` 兼容 alias；
- storage 使用自有 `Schema_id` / `Channel_id` / `Writer_options`，不暴露 mcap 类型；
- data metrics 布局内部化；
- `Broker::acquire_notification()` / `acquire_event()` 已提供；
- `Channel` 的 recovery / metrics 已移到 `Channel_advanced`；
- `shm` 实现移出公开头，改为动态库；
- base sink 实现内部化，公开工厂在 `base/log/backend.h`。

## 4. 已完成的拆分

### 4.1 ipc::Channel

已完成默认面拆分：

```text
ipc/channel.h             publish/subscribe/Reader
ipc/channel_advanced.h    恢复 + subscriber 观测
```

`Channel_advanced` 通过 friend 访问 Channel 的恢复/观测接口。

### 4.2 shm

已完成：实现移到 `valley/shm/src/shared_memory.cpp`，
公开头不再包含 Windows/POSIX OS 头；`shm` 由 header-only 改为动态库。

### 4.3 data metrics

已完成：`recorder_metrics.h` 移到 `valley/data/src/`，不再安装；
data 测试和 monitor 通过私有 include 读取原始布局。

### 4.4 Broker Notification

已完成：

```cpp
auto notification = broker.acquire_notification("FAULT");
auto event = broker.acquire_event("FAULT", "相机故障",
                                  {"安全带未系", "相机被遮挡"});
```

`Event::bind` 新增接收已有 `Notification&` 的重载，因此不再要求调用方持有
notification region。事件名列表仍需与 supervisor 配置顺序一致。

### 4.5 storage id

已完成：公开头使用 `storage::Schema_id` / `storage::Channel_id`，
`Writer_options` 和 `Compression` 也由 storage 提供，不再暴露 `mcap::*` 类型。

## 5. 最小使用面

大多数应用只需要：

```cpp
// 日志
vINFO(App) << "...";

// 数据
publisher.publish(data, size);
subscriber.set_handler(...);

// 事件
event.emit();
notification.wait_any_event();

// 任务
workset.bind_notification(&notification);
workset.poll_wait(tasks, timeout);

// 录制
recorder.record(std::move(trigger));
```

控制面：

```cpp
Supervisor / Broker
```

基础设施：

```cpp
serve::Server / serve::Client
```
