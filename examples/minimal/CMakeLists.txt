# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

cmake_minimum_required(VERSION 3.20)
project(valley_minimal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/utf-8)
endif()

find_package(ipc CONFIG REQUIRED)

add_executable(minimal_channel minimal_channel.cpp)
target_link_libraries(minimal_channel PRIVATE valley::ipc)

add_executable(minimal_event minimal_event.cpp)
target_link_libraries(minimal_event PRIVATE valley::ipc)

add_executable(minimal_synchronized minimal_synchronized.cpp)
target_link_libraries(minimal_synchronized PRIVATE valley::ipc)

set(valley_minimal_targets
  minimal_channel
  minimal_event
  minimal_synchronized)

# serve 的 package config 会 find_dependency(asio)；公开头不需要 asio，
# 因此 SDK 未附带 asio package 时用本地 shim 满足依赖。
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(serve CONFIG QUIET)
if (serve_FOUND)
  add_executable(minimal_serve minimal_serve.cpp)
  target_link_libraries(minimal_serve PRIVATE valley::serve)
  list(APPEND valley_minimal_targets minimal_serve)
else()
  message(STATUS "valley_minimal: serve package not found; "
                 "minimal_serve will not be built")
endif()

foreach (target ${valley_minimal_targets})
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endforeach()
