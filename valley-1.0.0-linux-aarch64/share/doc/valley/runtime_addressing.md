# 控制端点地址与动态库

## 1. 由 application 生成地址

Supervisor 和 Broker 使用同一套规则，由 `application` 名生成控制端点：

Windows：

```text
ipc://vIPC-CHN-<sanitized-application>
```

Linux：

```text
ipc:///tmp/vIPC-CHN-<sanitized-application>.sock
```

`sanitized-application` 只保留：

```text
a-z A-Z 0-9 - _
```

其他字符替换为 `_`。

### Supervisor

```cpp
exec::supervisor::Channel_config config;
config.application = "valley-drive";

exec::supervisor::Options options;
options.socket_dir = "/tmp";
```

地址自动为：

```text
ipc:///tmp/vIPC-CHN-valley-drive.sock
```

Supervisor 启动成功后记录：

```text
TAG=Supervisor endpoint ipc:///tmp/vIPC-CHN-valley-drive.sock
```

### Broker

```cpp
exec::broker::Broker_options options;
options.client_name = "realtime";
options.application = "valley-drive";
options.socket_dir = "/tmp";       // Windows 忽略
```

Broker 自动连接：

```text
ipc:///tmp/vIPC-CHN-valley-drive.sock
```

启动成功后记录：

```text
TAG=Broker supervisor endpoint ipc:///tmp/vIPC-CHN-valley-drive.sock
```

## 2. 显式地址覆盖

`Broker_options::addresses` 非空时优先使用显式地址：

```cpp
options.application = "valley-drive";
options.addresses = {"ipc://custom-endpoint"};
```

适用于测试、跨机器代理或自定义 socket path。

## 3. 地址生命周期

```text
supervisor 启动 -> 创建 / 监听 endpoint
broker start    -> 连接 endpoint
supervisor stop -> endpoint 释放
```

`application` 必须由 supervisor 和 broker 使用同一个值。

## 4. 动态库

项目库目标统一为动态库：

```text
base
base_tcp
shm
ipc
serve
exec
data
storage_mcap
storage_protobuf
ui
```

Windows：

```text
<target>.dll       -> bin/
<target>.lib       -> lib/
```

Linux：

```text
lib<target>.so     -> lib/
```

Linux 安装后通过：

```text
$ORIGIN/../lib
```

解析应用依赖，库之间通过 `$ORIGIN` 互相解析。

## 5. 构建产物

构建树：

```text
build/<os>_<arch>_<type>/bin
build/<os>_<arch>_<type>/lib
```

安装树：

```text
out/<os>_<arch>_<type>/bin
out/<os>_<arch>_<type>/lib
```

测试在构建树运行；安装树用于发布。
