# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

cmake_minimum_required(VERSION 3.20)
project(valley_drive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# serveConfig.cmake 会 find_dependency(asio)；公开头不需要 asio，
# 因此 SDK 未附带 asio package 时用本地 shim 满足依赖。
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(data REQUIRED)

if (NOT TARGET asio::asio)
  find_package(asio CONFIG QUIET)
endif()
if (NOT TARGET asio::asio)
  find_path(VALLEY_ASIO_INCLUDE asio.hpp)
  if (NOT VALLEY_ASIO_INCLUDE)
    message(FATAL_ERROR
      "asio not found: pass -DVALLEY_ASIO_INCLUDE=<asio include dir>")
  endif()
  add_library(asio::asio INTERFACE IMPORTED)
  target_include_directories(asio::asio INTERFACE ${VALLEY_ASIO_INCLUDE})
endif()

find_package(exec REQUIRED)
find_package(ipc REQUIRED)

add_executable(valley_drive
  main.cpp
  sensor/camera_device.cpp
  sensor/imu_device.cpp
  sensor/lidar_device.cpp)

target_include_directories(valley_drive PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/common)

target_link_libraries(valley_drive PRIVATE
  valley::data
  valley::exec
  valley::ipc
  valley::serve
  Threads::Threads)

if (MSVC)
  target_compile_options(valley_drive PRIVATE /W4 /utf-8)
else()
  target_compile_options(valley_drive PRIVATE -Wall -Wextra)
endif()

install(TARGETS valley_drive RUNTIME DESTINATION bin)
install(FILES topology.json DESTINATION conf)
