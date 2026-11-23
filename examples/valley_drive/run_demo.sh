#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

# valley_drive multi-process demo: supervisor / subscriber / recorder / publisher / monitor
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/../.." && pwd)"
arch_name="$(uname -m)"

is_valley_sdk() {
    [[ -n "${1:-}" && -f "$1/lib/cmake/ipc/ipcConfig.cmake" ]]
}

release_arg="${1:-}"
duration="${2:-30}"
if (( duration < 10 )); then
    echo "duration < 10s; using 10s so the recorder trigger can finish" >&2
    duration=10
fi

# 1. 找 SDK：优先 VALLEY_SDK，其次仓库 dist/ 下当前架构的 Linux SDK。
sdk="${VALLEY_SDK:-}"
if ! is_valley_sdk "$sdk" && [[ -d "$root/dist" ]]; then
    for candidate in "$root"/dist/valley-*-linux-"$arch_name"; do
        if is_valley_sdk "$candidate"; then
            sdk="$candidate"
        fi
    done
fi

# 2. 找运行目录：第一个参数 > dist SDK > 仓库根 package 布局 > 旧 out 布局。
if [[ -n "$release_arg" ]]; then
    release_dir="$release_arg"
elif is_valley_sdk "$sdk"; then
    release_dir="$sdk"
elif [[ -x "$root/bin/supervisor" ]]; then
    release_dir="$root"
elif [[ -d "$root/out/Linux_${arch_name}_Release" ]]; then
    release_dir="$root/out/Linux_${arch_name}_Release"
else
    echo "cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/" >&2
    exit 1
fi
if [[ "$release_dir" != /* ]]; then
    release_dir="$root/$release_dir"
fi
if [[ ! -d "$release_dir" ]]; then
    echo "release directory not found: $release_dir" >&2
    exit 1
fi

bin_dir="$release_dir/bin"
supervisor="$bin_dir/supervisor"
monitor="$bin_dir/monitor"
if [[ ! -x "$supervisor" ]]; then
    echo "missing supervisor: $supervisor" >&2
    exit 1
fi

# 3. 找示例可执行文件。SDK 包通常不带 valley_drive，需要单独 build。
exe="${3:-${VALLEY_DRIVE_EXE:-}}"
if [[ -z "$exe" ]]; then
    for candidate in \
        "$bin_dir/valley_drive" \
        "$root/build/valley_drive_linux/valley_drive" \
        "$root/build/valley_drive/valley_drive" \
        "$root/examples/valley_drive/build/valley_drive"; do
        if [[ -x "$candidate" ]]; then
            exe="$candidate"
            break
        fi
    done
fi
if [[ -n "$exe" && "$exe" != /* ]]; then
    exe="$root/$exe"
fi
if [[ -z "$exe" || ! -x "$exe" ]]; then
    echo "missing valley_drive; build examples/valley_drive or pass it as arg 3" >&2
    exit 1
fi
echo "example: $exe"
echo "sdk: $release_dir"

# 运行库从 SDK 的 lib 加载；不要修改 SDK 目录本身。
export LD_LIBRARY_PATH="$release_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

run_base="$PWD"
run_dir="$run_base/valley_drive_$(date +%Y%m%d_%H%M%S)_$$"
mkdir -p "$run_dir/conf"
topology="$run_dir/conf/topology.json"
install -m 644 "$script_dir/topology.json" "$topology"
application="$(sed -n 's/.*"application"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$topology" | head -n 1)"
if [[ -z "$application" ]]; then
    echo "topology.json is missing application" >&2
    exit 1
fi
echo "config: $topology"
echo "application: $application"

pids=()
cleanup() {
    for pid in "${pids[@]:-}"; do
        kill "$pid" 2>/dev/null || true
    done
}
trap cleanup EXIT INT TERM

"$supervisor" --config "$topology" \
    >"$run_dir/supervisor.out" 2>"$run_dir/supervisor.err" &
pids+=("$!")
for _ in $(seq 1 50); do
    if grep -q "supervisor endpoint:" "$run_dir/supervisor.err" 2>/dev/null; then
        break
    fi
    sleep 0.1
done
if ! grep -q "supervisor endpoint:" "$run_dir/supervisor.err" 2>/dev/null; then
    echo "supervisor did not become ready" >&2
    exit 1
fi

"$exe" subscriber --application "$application" --duration "$duration" \
    >"$run_dir/subscriber.out" 2>"$run_dir/subscriber.err" &
pids+=("$!")

"$exe" recorder --application "$application" --duration "$duration" \
    --trigger-after 5 --storage "$run_dir" \
    >"$run_dir/recorder.out" 2>"$run_dir/recorder.err" &
pids+=("$!")

"$exe" publisher --application "$application" --duration "$duration" \
    >"$run_dir/publisher.out" 2>"$run_dir/publisher.err" &
pids+=("$!")

monitor_mode="${MONITOR:-auto}"
if [[ "$monitor_mode" == "auto" ]]; then
    if [[ -t 0 && -t 1 ]]; then
        monitor_mode=1
    else
        monitor_mode=0
    fi
fi

if [[ "$monitor_mode" == "1" && -x "$monitor" ]]; then
    echo "monitor: $monitor (press q to exit)"
    # UI must stay attached to the terminal; redirecting it hides the UI.
    "$monitor" --application "$application" || true
else
    echo "running for $duration seconds; dir=$run_dir"
    sleep "$duration"
    "$monitor" --application "$application" --once || true
    sleep 2
fi
echo "run dir: $run_dir"
find "$run_dir" -maxdepth 1 -name '*.mcap' -printf '%f\n'
