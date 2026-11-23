# Valley SDK

鏈寘鍙寘鍚?Valley 鐨勪簩杩涘埗銆佸叕寮€澶存枃浠跺拰 CMake 閰嶇疆锛?

`	ext
bin/
lib/
include/
lib/cmake/
`

浣跨敤鏂囨。銆佺ず渚嬪拰 benchmark 浣嶄簬鐙珛鐨?valley-sdk 浠撳簱銆?

shm 涓嶄綔涓?SDK 鍙戝竷闈細涓嶆彁渚?shm 澶存枃浠躲€丆Make 閰嶇疆鍜?Windows 瀵煎叆搴撱€?
monitor/supervisor 杩愯鏃朵粛闇€瑕?shm 鍔ㄦ€佸簱锛屽洜姝ゅ寘鍐呬繚鐣欏叾杩愯鏃朵簩杩涘埗銆?

ui 涓?FTXUI 鍙湇鍔′簬 monitor锛屽悓鏍蜂笉鏄?SDK 妯″潡锛涘寘鍐呭彧淇濈暀 UI 杩愯鏃跺簱銆?

## 浣跨敤

`ash
cmake -S your-project -B build \
  -DCMAKE_PREFIX_PATH=/path/to/this/package
`

`cmake
find_package(ipc CONFIG REQUIRED)
target_link_libraries(your-target PRIVATE valley::ipc)
`