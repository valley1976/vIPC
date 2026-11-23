#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/.." && pwd)"
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

# SDK packages can be extracted under dist/valley-<version>-<platform>.
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

build_dir="${VALLEY_BUILD_DIR:-$script_dir/build}"
cmake -S "$script_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$sdk"
cmake --build "$build_dir" --parallel
export LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

for name in channel_bench event_bench synchronized_bench serve_bench; do
    exe="$build_dir/$name"
    if [[ ! -x "$exe" ]]; then
        echo "skip $name (not built)"
        continue
    fi
    echo "running $name"
    "$exe"
done
