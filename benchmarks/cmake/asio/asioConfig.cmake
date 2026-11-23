# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

# 最小 asio package config shim。
#
# serve 的公开头（include/serve/*.h）不包含 asio，只有 .dll/.lib 的实现内部
# 使用它；但 serveConfig.cmake 仍会 find_dependency(asio)。当 SDK 没有附带
# asio 的 CMake package 时，这个 shim 只提供一个空的 asio::asio 目标，让
# find_package(serve) 能继续。需要编译 serve 内部头时通过
# -DVALLEY_ASIO_INCLUDE=<asio include 目录> 传入真实头路径。

if(NOT TARGET asio::asio)
  add_library(asio::asio INTERFACE IMPORTED)
  if(DEFINED VALLEY_ASIO_INCLUDE AND VALLEY_ASIO_INCLUDE)
    set_target_properties(asio::asio PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${VALLEY_ASIO_INCLUDE}")
  endif()
endif()

set(asio_FOUND TRUE)
