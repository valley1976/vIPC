#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/.." && pwd)"
sdk="${VALLEY_SDK:-}"

if [[ -z "$sdk" ]]; then
    echo "verify.sh: set VALLEY_SDK to an extracted release package" >&2
    exit 1
fi
if [[ ! -f "$sdk/lib/cmake/ipc/ipcConfig.cmake" ]]; then
    echo "verify.sh: not a Valley SDK: $sdk" >&2
    exit 1
fi

echo "verify: SDK=$sdk"
VALLEY_SDK="$sdk" "$root/examples/minimal/build.sh"
VALLEY_SDK="$sdk" "$root/benchmarks/build.sh"
echo "verify: OK"
