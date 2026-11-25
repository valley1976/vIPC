# Valley SDK

This package contains Valley binaries, public headers and CMake configs:

```text
bin/
lib/
include/
lib/cmake/
```

User documentation, examples and benchmarks live in the separate valley-sdk repository.

shm is not part of the SDK surface: no shm headers, CMake config or Windows import library.
monitor/supervisor still need the shm runtime DLL, so the runtime binary is kept.

ui and FTXUI only serve monitor; only the UI runtime library is kept.

## Usage

```bash
cmake -S your-project -B build \
  -DCMAKE_PREFIX_PATH=/path/to/this/package
```

```cmake
find_package(ipc CONFIG REQUIRED)
target_link_libraries(your-target PRIVATE valley::ipc)
```