# 快速开始

本文面向第一次使用 Valley 的开发者。完整设计见
[architecture.md](architecture.md)，公开 API 清单见 [api_surface.md](api_surface.md)。

## 1. 获取 SDK

Release 包包含：

```text
bin/       应用和动态库
lib/       导入库 / .so / .a
include/   公开头
lib/cmake/ find_package 配置
README.md
NOTICE
```

Linux 构建：

```bash
./scripts/build.sh -p all --test
./scripts/build.sh -p data --test
./scripts/build.sh -p storage --test
```

Windows：

```powershell
scripts\build.ps1 -Target all -RunTests
scripts\build.ps1 -Target data -RunTests
scripts\build.ps1 -Target storage -RunTests
```

## 2. CMake 接入

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ipc CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE valley::ipc)
```

配置时指定 SDK：

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/valley-release
cmake --build build
```

Linux 运行：

```bash
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/my_app
```

Windows 运行：

```powershell
$env:PATH = "C:\path\to\valley-release\bin;$env:PATH"
.\build\my_app.exe
```

## 3. 最小 Channel 示例

```cpp
#include "ipc/channel.h"

#include <cstdint>
#include <cstdio>

int main()
{
    ipc::Channel channel{ipc::Channel::Config{8, 1}};

    auto subscriber = channel.subscribe();
    auto reader = subscriber->make_reader();

    if (!channel.publish(std::uint64_t{42}))
        return 1;

    if (reader->try_read() && reader->value() == 42)
        std::puts("received 42");
    reader->unpin();
}
```

完整示例：[examples/minimal/minimal_channel.cpp](../examples/minimal/minimal_channel.cpp)

## 4. 最小 Event 示例

```cpp
#include "ipc/event.h"
#include "ipc/notification.h"

#include <chrono>
#include <cstddef>

int main()
{
    alignas(64) std::byte storage[256]{};

    auto notification = ipc::Notification::create(
        storage, sizeof storage, "FAULT", {"camera"});

    ipc::Event::bind("FAULT", notification);

    ipc::Event event{"FAULT", "camera"};
    event.emit();

    if (notification.wait(std::chrono::milliseconds{100}))
        (void)notification.poll();
}
```

完整示例：[examples/minimal/minimal_event.cpp](../examples/minimal/minimal_event.cpp)

## 5. Supervisor + Broker

拓扑：

```jsonc
{
  "application": "my-app",
  "channel": [
    {
      "group": "/sensor",
      "topic": [
        {"name": "/sensor/data", "data_size": 4096}
      ]
    }
  ]
}
```

启动 supervisor：

```bash
./bin/supervisor --config topology.json
```

Linux 自动生成：

```text
ipc:///tmp/vIPC-CHN-my-app.sock
```

Windows：

```text
ipc://vIPC-CHN-my-app
```

Broker：

```cpp
exec::broker::Broker_options options;
options.client_name = "realtime";
options.application = "my-app";

exec::broker::Broker::instance().start(options);

auto publisher = exec::broker::Broker::instance()
                     .acquire_publisher("/sensor/data");
```

## 6. 常见问题

### 找不到 .so

设置：

```bash
export LD_LIBRARY_PATH=$SDK/lib:$LD_LIBRARY_PATH
```

或安装时保留 RPATH。

### 找不到 .dll

把 SDK 的 `bin` 加入 `PATH`。

### 端点连不上

确认 supervisor 和 Broker 使用同一个 `application`：

```text
Linux:   /tmp/vIPC-CHN-<application>.sock
Windows: vIPC-CHN-<application>
```

### 不要包含私有头

只使用：

```text
include/ 下的公开头
```

不要包含 `src/`、`detail/` 下的内部头。
