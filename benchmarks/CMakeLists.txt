# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

cmake_minimum_required(VERSION 3.20)
project(valley_sdk_benchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(ipc CONFIG REQUIRED)

function(valley_add_benchmark target)
  add_executable(${target} ${target}.cpp)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_link_libraries(${target} PRIVATE ${ARGN} Threads::Threads)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endfunction()

valley_add_benchmark(channel_bench valley::ipc)
valley_add_benchmark(event_bench valley::ipc)
valley_add_benchmark(synchronized_bench valley::ipc)

# serve 的 package config 会 find_dependency(asio)；公开头不需要 asio，
# 因此 SDK 未附带 asio package 时用本地 shim 满足依赖。
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(serve CONFIG QUIET)
if (serve_FOUND)
  valley_add_benchmark(serve_bench valley::serve)
else()
  message(STATUS "valley_sdk_benchmarks: serve package not found; "
                 "serve_bench will not be built")
endif()
