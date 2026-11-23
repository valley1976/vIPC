#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/../.." && pwd)"
arch_name="$(uname -m)"

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

if ! is_valley_sdk "$sdk" && [[ -d "$root/dist" ]]; then
    for candidate in "$root"/dist/valley-*-linux-"$arch_name"; do
        if is_valley_sdk "$candidate"; then
            sdk="$candidate"
        fi
    done
fi

if ! is_valley_sdk "$sdk"; then
    candidate="$root/out/Linux_${arch_name}_Release"
    if is_valley_sdk "$candidate"; then
        sdk="$candidate"
    fi
fi

if ! is_valley_sdk "$sdk"; then
    echo "build.sh: cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/" >&2
    exit 1
fi
sdk="$(cd "$sdk" && pwd)"

build_dir="${VALLEY_BUILD_DIR:-$root/build/valley_drive}"

prefix_path="$sdk"
if [[ -n "${VALLEY_CMAKE_PREFIX_PATH:-}" ]]; then
    prefix_path="$prefix_path;$VALLEY_CMAKE_PREFIX_PATH"
fi

cmake_args=(
    -S "$script_dir"
    -B "$build_dir"
    -DCMAKE_BUILD_TYPE=Release
    "-DCMAKE_PREFIX_PATH=$prefix_path"
)
if [[ -n "${VALLEY_CONAN_TOOLCHAIN:-}" ]]; then
    cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$VALLEY_CONAN_TOOLCHAIN")
fi
if [[ -n "${VALLEY_ASIO_INCLUDE:-}" ]]; then
    cmake_args+=("-DVALLEY_ASIO_INCLUDE=$VALLEY_ASIO_INCLUDE")
fi

if ! cmake "${cmake_args[@]}"; then
    echo "build.sh: cmake configure failed; set VALLEY_CMAKE_PREFIX_PATH to dependency configs or VALLEY_CONAN_TOOLCHAIN" >&2
    exit 1
fi
cmake --build "$build_dir" --parallel

exe="$build_dir/valley_drive"
if [[ -x "$exe" ]]; then
    echo "built: $exe"
else
    echo "built in: $build_dir"
fi
