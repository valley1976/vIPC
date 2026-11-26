# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

# Minimal mcap package config shim for the SDK examples.
#
# The SDK ships storage_mcap as a self-contained shared library; its public
# headers do not expose mcap types, but storage_mcapConfig.cmake still calls
# find_dependency(mcap). This shim provides the empty mcap::mcap target that
# the imported storage_mcap target expects.

if(NOT TARGET mcap::mcap)
  add_library(mcap::mcap INTERFACE IMPORTED)
endif()

set(mcap_FOUND TRUE)
