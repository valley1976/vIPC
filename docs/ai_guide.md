# AI / 新维护者上下文

本文件给自动化代理和新加入的维护者使用。

## 阅读顺序

```text
1. docs/getting_started.md
2. docs/api_surface.md
3. docs/architecture.md
4. docs/time_design.md
5. docs/event_design.md
6. docs/notification_pool_design.md
7. docs/synchronized_design.md
8. docs/testing.md
9. docs/test_plan.md
```

## 公开 API

只使用 `include/` 下的头：

```text
base/log
ipc/channel.h
ipc/channel_advanced.h
ipc/event.h
ipc/notification.h
ipc/synchronized.h
ipc/synchronized_pool.h
time/time.h
exec/cyclic_task.hpp
exec/workset.hpp
exec/broker/broker.h
exec/supervisor/supervisor.h
exec/supervisor/config.h
data/recorder.h
storage/mcap_writer.h
serve/serve.h
shm/shared_memory.hpp
```

不要包含：

```text
src/
detail/
internal/
```

## 不变式

```text
Event.emit() 只置位并 signal，不执行 handler
Notification.wait() 等待内部 auto-reset event
Workset event index == task index
同一 notification 重复 set 不产生第二个 token
Notification pool 只在 supervisor 内部构建
Broker.application 与 supervisor.application 必须一致
```

## 代码约束

- C++17；
- 类型：`Proc_info`、`Ui_state` 风格；
- 函数/变量：小写下划线；
- 私有成员尾随 `_`；
- 新代码文件必须带版权头；
- 不提交生成文件、`build/`、`out/`、`CMakeUserPresets.json`。

## 测试

```bash
./scripts/build.sh -p base --test
./scripts/build.sh -p shm --test
./scripts/build.sh -p ipc --test
./scripts/build.sh -p storage --test
./scripts/build.sh -p data --test
./scripts/build.sh -p exec --test
./scripts/build.sh -p serve --test
./scripts/build.sh -p tools --test
```

Windows：

```powershell
scripts\build.ps1 -Target tools -RunTests
```

## 调试入口

```text
Channel              ipc/channel.h
Event/Notification   ipc/event.h, ipc/notification.h
Workset              exec/workset.hpp
Supervisor/Broker    exec/supervisor, exec/broker
Record               data/recorder.h
Shared memory        shm/shared_memory.hpp
RPC                  serve/serve.h
```
