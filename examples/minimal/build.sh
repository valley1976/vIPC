#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/../.." && pwd)"
host_arch="$(uname -m)"

is_valley_sdk() {
    [[ -n "${1:-}" && -f "$1/lib/cmake/ipc/ipcConfig.cmake" ]]
}

sdk="${VALLEY_SDK:-}"
if ! is_valley_sdk "$sdk"; then
    for candidate in "$root/sdk" "$root"; do
        if is_valley_sdk "$candidate"; then
            sdk="$candidate"
            break
        fi
    done
fi

# 仓库里的 dist/ 存放各平台 SDK，例如：
#   dist/valley-1.0.0-linux-x86_64
#   dist/valley-1.0.0-windows-x86_64
if ! is_valley_sdk "$sdk" && [[ -d "$root/dist" ]]; then
    for candidate in "$root"/dist/valley-*-linux-"$host_arch"; do
        if is_valley_sdk "$candidate"; then
            sdk="$candidate"
        fi
    done
fi

if ! is_valley_sdk "$sdk" && [[ -d "$root/out" ]]; then
    for candidate in "$root"/out/Linux_"$host_arch"_Release "$root"/out/*_Release; do
        if is_valley_sdk "$candidate"; then
            sdk="$candidate"
            break
        fi
    done
fi

if ! is_valley_sdk "$sdk"; then
    echo "build.sh: cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/" >&2
    exit 1
fi
sdk="$(cd "$sdk" && pwd)"

build_dir="${VALLEY_BUILD_DIR:-$script_dir/build-${host_arch}}"

cmake -S "$script_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$sdk"
cmake --build "$build_dir" --parallel

export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$build_dir/minimal_channel"
"$build_dir/minimal_event"
"$build_dir/minimal_synchronized"
if [[ -x "$build_dir/minimal_serve" ]]; then
    "$build_dir/minimal_serve"
else
    echo "skip minimal_serve (serve package not found)"
fi
