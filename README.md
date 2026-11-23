# Valley SDK

Valley 的独立使用者仓库：用户文档、示例和 benchmark。

核心实现仓库只保留库实现、测试、设计文档和发布脚本。本仓库只通过发布产物
使用 Valley：

```text
find_package(...)
Conan package
dist/valley-<version>-<platform>.<ext>
```

不包含、也不引用 Valley 源码树。

## 目录

```text
docs/          使用者文档
examples/      可独立编译的示例
benchmarks/    基于公开 API 的性能基准
ci/            SDK 验证脚本
dist/          本地 SDK（已忽略，不提交）
```

## 获取 SDK

从核心仓库的 release 产物获取：

```text
valley-<version>-linux-x86_64.tar.gz
valley-<version>-linux-aarch64.tar.gz
valley-<version>-windows-x86_64.zip
```

解压到仓库根目录的 `dist/` 下（`dist/` 已忽略，不提交）：

```text
dist/valley-<version>-<platform>/
```

也可以设置 `VALLEY_SDK` 指向任意解压目录：

```bash
export VALLEY_SDK=/path/to/valley-<version>-<platform>
```

Windows：

```powershell
$env:VALLEY_SDK = 'C:\path\to\valley-<version>-windows-x86_64'
```

示例 build 脚本会优先使用 `VALLEY_SDK`，否则自动查找 `dist/`。

## 构建示例

```bash
cd examples/minimal
./build.sh
```

Windows：

```powershell
cd examples\minimal
.\build.ps1
```

`build.sh` / `build.ps1` 会自动尝试：

```text
VALLEY_SDK
仓库根下的 dist/valley-*-<platform>
仓库根下的 sdk/ 目录
仓库根下的 out/*_Release
```

`examples/minimal` 包含 `channel`、`event`、`Synchronized`、`serve` 四个最小示例；
其中 `minimal_serve` 只在 SDK 提供 `serve` package 时构建。

从仓库根也可以构建全部示例：

```bash
./examples/build.sh
```

```powershell
.\examples\build.ps1
```

## 构建 / 运行 valley_drive 示例

`valley_drive` 依赖 `data` / `exec` / `ipc`，先构建：

```powershell
$env:VALLEY_CMAKE_PREFIX_PATH = 'C:\path\to\conan\generators'
.\examples\valley_drive\build.ps1
```

Linux：

```bash
export VALLEY_CMAKE_PREFIX_PATH=/path/to/conan/generators
./examples/valley_drive/build.sh
```

也可以设置 `VALLEY_CONAN_TOOLCHAIN` 指向 Conan toolchain；依赖细节见
[examples/valley_drive/README.md](examples/valley_drive/README.md)。

然后运行：

```powershell
powershell -ExecutionPolicy Bypass -File `
  examples\valley_drive\run_demo.ps1 -Duration 30
```

Linux：

```bash
examples/valley_drive/run_demo.sh "" 30
```

运行脚本会自动从 `dist/` 查找 SDK 的 `supervisor` / `monitor`，从
`build/valley_drive*` 查找示例可执行文件；topology 和录制产物都写到本次运行
目录，不会修改 SDK 目录。endpoint 由 `topology.json` 的 `application` 决定，
`valley_drive` 与 supervisor 使用同一规则，不再手写 `--address`。

## Benchmark

```bash
cd benchmarks
./build.sh                 # 编译并依次运行全部 benchmark
./build/channel_bench      # 也可以单独运行
./build/event_bench
./build/synchronized_bench
./build/serve_bench
```

Windows：

```powershell
cd benchmarks
.\build.ps1
```

`build.ps1` 会编译并依次运行；VS 多配置生成器的可执行文件在 `build\Release\` 下。
benchmark build 脚本也会按与示例相同的规则查找 `dist/` 里的 SDK。

| benchmark | 内容 |
|---|---|
| `channel_bench` | `ipc::Channel` publish + 每个 subscriber read 的吞吐 |
| `event_bench` | `ipc::Event` emit 吞吐、同线程 emit+wait、跨线程唤醒延迟 |
| `synchronized_bench` | `ipc::Synchronized` 的 apply / try_apply 开销与竞争吞吐 |
| `serve_bench` | `serve::Server` / `serve::Client` RPC 往返时延（IPC / TCP） |

benchmark 只使用公开头，不引用核心源码。各 benchmark 的参数见
[benchmarks/README.md](benchmarks/README.md)。`serve_bench` 需要 SDK 提供
`serve` package；没有时构建会跳过它。当前 Windows 机器的一组实测结果见
[docs/benchmark_results.md](docs/benchmark_results.md)。

## 验证

```bash
VALLEY_SDK=/path/to/sdk ./ci/verify.sh
```

验证内容：

```text
find_package
minimal example 编译运行
benchmark 编译运行
```

## 版本

示例和 benchmark 应固定到某个 Valley release 版本。升级 SDK 时同步更新
`docs/` 和 `ci/` 中的版本说明。
