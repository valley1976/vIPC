# Minimal SDK Examples

这些示例只依赖已安装的公开头：

| 示例 | 依赖 | 内容 |
|---|---|---|
| `minimal_channel` | `valley::ipc` | `ipc::Channel` publish / read |
| `minimal_event` | `valley::ipc` | `ipc::Event` + `ipc::Notification` |
| `minimal_synchronized` | `valley::ipc` | `ipc::Synchronized<T>` apply |
| `minimal_serve` | `valley::serve` | `serve::Server` / `serve::Client` echo |

`minimal_serve` 只在 SDK 提供 `serve` package 时构建；否则构建脚本会跳过它。

## Linux

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/valley-release
cmake --build build
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_channel
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_event
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_synchronized
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_serve
```

## Windows

```powershell
cmake -S . -B build `
  -DCMAKE_PREFIX_PATH=C:\path\to\valley-release
cmake --build build --config Release
$env:PATH = "C:\path\to\valley-release\bin;$env:PATH"
.\build\Release\minimal_channel.exe
.\build\Release\minimal_event.exe
.\build\Release\minimal_synchronized.exe
.\build\Release\minimal_serve.exe
```

## 一键构建

脚本会自动查找：

```text
VALLEY_SDK
仓库 dist/valley-*-<platform>   例如 dist/valley-1.0.0-windows-x86_64
仓库 sdk/
仓库 out/*_Release
```

```bash
./build.sh
```

Windows：

```powershell
.\build.ps1
```

脚本会依次运行全部已构建示例；`minimal_serve` 未构建时显示 skip。

也可以显式指定 SDK：

```bash
VALLEY_SDK=/path/to/valley-release ./build.sh
```

```powershell
$env:VALLEY_SDK = 'C:\path\to\valley-release'
.\build.ps1
```

构建目录可以用 `VALLEY_BUILD_DIR` 覆盖；默认 Linux 为 `build-<arch>`，
Windows 为 `build-windows`。

仓库根也提供总入口：

```bash
./examples/build.sh
```

```powershell
.\examples\build.ps1
```

它会先构建/运行 minimal，再构建 valley_drive；valley_drive 的依赖配置见
[examples/valley_drive/README.md](../valley_drive/README.md)。
