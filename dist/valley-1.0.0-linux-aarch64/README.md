# Valley SDK

本包只包含 Valley 的二进制、公开头文件和 CMake 配置：

```text
bin/
lib/
include/
lib/cmake/
```

使用文档、示例和 benchmark 位于独立的 valley-sdk 仓库。

shm 不作为 SDK 发布面：不提供 shm 头文件、CMake 配置和 Windows 导入库。
monitor/supervisor 运行时仍需要 shm 动态库，因此包内保留其运行时二进制。

ui 与 FTXUI 只服务于 monitor，同样不是 SDK 模块；包内只保留 UI 运行时库。

## 使用

```bash
cmake -S your-project -B build \
  -DCMAKE_PREFIX_PATH=/path/to/this/package
```

```cmake
find_package(ipc CONFIG REQUIRED)
target_link_libraries(your-target PRIVATE valley::ipc)
```
