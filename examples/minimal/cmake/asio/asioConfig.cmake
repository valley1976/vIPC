# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

# Minimal asio package config shim for the SDK examples.
#
# serve's public headers do not include asio, but serveConfig.cmake still calls
# find_dependency(asio). This shim provides an empty asio::asio target when the
# SDK does not ship an asio CMake package. Set VALLEY_ASIO_INCLUDE only when
# compiling serve's internal headers is required.

if(NOT TARGET asio::asio)
  add_library(asio::asio INTERFACE IMPORTED)
  if(DEFINED VALLEY_ASIO_INCLUDE AND VALLEY_ASIO_INCLUDE)
    set_target_properties(asio::asio PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${VALLEY_ASIO_INCLUDE}")
  endif()
endif()

set(asio_FOUND TRUE)
