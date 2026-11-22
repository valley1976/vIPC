# Minimal SDK Examples

这两个示例只依赖已安装的公开头与 `valley::ipc`。

## Linux

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/valley-release
cmake --build build
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_channel
LD_LIBRARY_PATH=/path/to/valley-release/lib ./build/minimal_event
```

## Windows

```powershell
cmake -S . -B build `
  -DCMAKE_PREFIX_PATH=C:\path\to\valley-release
cmake --build build --config Release
$env:PATH = "C:\path\to\valley-release\bin;$env:PATH"
.\build\Release\minimal_channel.exe
.\build\Release\minimal_event.exe
```


## 一键构建

脚本会自动查找发布包根目录或源码树 `out/*_Release`：

```bash
./build.sh
```

Windows：

```powershell
.uild.ps1
```

也可以显式指定 SDK：

```bash
VALLEY_SDK=/path/to/valley-release ./build.sh
```

```powershell
$env:VALLEY_SDK = 'C:\path\to\valley-release'
.\build.ps1
```
