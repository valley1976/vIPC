#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sdk="${VALLEY_SDK:-}"
if [[ -z "$sdk" ]]; then
    candidate="$script_dir/../.."
    if [[ -f "$candidate/lib/cmake/ipc/ipcConfig.cmake" ]]; then
        sdk="$candidate"
    fi
fi
host_arch="$(uname -m)"
if [[ -z "$sdk" ]]; then
    candidate="$script_dir/../../out/Linux_${host_arch}_Release"
    if [[ -f "$candidate/lib/cmake/ipc/ipcConfig.cmake" ]]; then
        sdk="$candidate"
    fi
fi
if [[ -z "$sdk" ]]; then
    for candidate in "$script_dir"/../../out/*_Release; do
        if [[ -f "$candidate/lib/cmake/ipc/ipcConfig.cmake" ]]; then
            sdk="$candidate"
            break
        fi
    done
fi
if [[ -z "$sdk" ]]; then
    echo "build.sh: cannot find Valley SDK; set VALLEY_SDK" >&2
    exit 1
fi

build_dir="${VALLEY_BUILD_DIR:-$script_dir/build-${host_arch}}"

cmake -S "$script_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$sdk"
cmake --build "$build_dir" --parallel

export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$build_dir/minimal_channel"
"$build_dir/minimal_event"
