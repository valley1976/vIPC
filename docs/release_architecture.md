# Valley 架构概览

> 发布包中的简要说明。完整设计保留在源码仓库。

## 分层

```text
应用
  │
  ├─ exec：Broker、Supervisor、Workset、周期任务
  ├─ ipc：Channel、Event、Notification
  ├─ serve：控制面 RPC
  ├─ data / storage：录制与 MCAP
  └─ shm / base
```

## 数据流

```text
publisher
  -> Broker
  -> Channel
  -> subscriber / Recorder
```

- `Broker` 是客户端访问数据面的入口；
- `Channel` 负责 topic 的数据传递；
- `Recorder` 消费 topic 并写入 MCAP。

## 控制流

```text
application
  -> Supervisor
  -> Broker
```

- `Supervisor` 根据拓扑配置启动数据面；
- `Broker` 按 `application` 找到并连接 Supervisor；
- 客户端通过 Broker 获取 topic 访问能力。

## 事件与任务

```text
Event
  -> Notification
  -> Workset
  -> task handler
```

- `Event` 表示事件发生；
- `Notification` 负责事件等待；
- `Workset` 将事件映射到任务执行。

## 模块职责

| 模块 | 职责 |
|---|---|
| `base` | 日志、字符串、基础工具 |
| `time` | UTC/steady 时间与同步 |
| `shm` | 命名共享内存 |
| `ipc` | Channel、Event、Notification |
| `serve` | 控制面通信与 RPC |
| `exec` | Broker、Supervisor、Workset |
| `data` | Recorder、Trigger |
| `storage` | MCAP 写入 |
| `ui` | 终端监控 |

## 设计原则

```text
控制面与数据面分离
topic 数据走 Channel
事件只表达状态变化，不携带业务数据
Broker 是客户端统一入口
```

## 平台

对外 API 为 C++17，库以动态库发布，支持 Windows 和 Linux。
