#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

# valley_drive multi-process demo: supervisor / subscriber / recorder / publisher / monitor
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$script_dir/../.." && pwd)"
package_layout=0
if [[ -x "$root/bin/supervisor" ]]; then
    package_layout=1
fi
arch_name="$(uname -m)"
release_dir="${1:-$root/out/Linux_${arch_name}_Release}"
if (( package_layout == 1 )); then
    release_dir="${1:-$root}"
fi
duration="${2:-30}"
if (( duration < 10 )); then
    echo "duration < 10s; using 10s so the recorder trigger can finish" >&2
    duration=10
fi
if [[ "$release_dir" != /* ]]; then
    release_dir="$root/$release_dir"
fi

exe="${3:-${VALLEY_DRIVE_EXE:-}}"
if [[ -z "$exe" ]]; then
    for candidate in \
        "$release_dir/bin/valley_drive" \
        "$root/build/valley_drive_linux/valley_drive" \
        "$root/build/valley_drive/valley_drive"; do
        if [[ -x "$candidate" ]]; then
            exe="$candidate"
            break
        fi
    done
fi
if [[ -n "$exe" && "$exe" != /* ]]; then
    exe="$root/$exe"
fi
bin_dir="$release_dir/bin"
mkdir -p "$bin_dir"
bin_exe="$bin_dir/valley_drive"
if [[ -n "$exe" && "$exe" != "$bin_exe" ]]; then
    install -m 755 "$exe" "$bin_exe"
fi
if [[ -x "$bin_exe" ]]; then
    exe="$bin_exe"
fi
echo "example: $exe"
conf_dir="$release_dir/conf"
mkdir -p "$conf_dir"
topology="$conf_dir/topology.json"
install -m 644 "$script_dir/topology.json" "$topology"
echo "config: $topology"
supervisor="$bin_dir/supervisor"
monitor="$bin_dir/monitor"
for path in "$exe" "$supervisor"; do
    if [[ -z "$path" || ! -x "$path" ]]; then
        echo "missing executable: $path (pass example path as arg 3)" >&2
        exit 1
    fi
done

application="valley-drive"
address="ipc:///tmp/vIPC-CHN-$application.sock"
run_base="$PWD"
run_dir="$run_base/valley_drive_$(date +%Y%m%d_%H%M%S)_$$"
mkdir -p "$run_dir"
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

"$exe" subscriber --address "$address" --duration "$duration" \
    >"$run_dir/subscriber.out" 2>"$run_dir/subscriber.err" &
pids+=("$!")

"$exe" recorder --address "$address" --duration "$duration" \
    --trigger-after 5 --storage "$run_dir" \
    >"$run_dir/recorder.out" 2>"$run_dir/recorder.err" &
pids+=("$!")

"$exe" publisher --address "$address" --duration "$duration" \
    >"$run_dir/publisher.out" 2>"$run_dir/publisher.err" &
pids+=("$!")

if [[ "${MONITOR:-0}" == "1" && -x "$monitor" ]]; then
    "$monitor" --address "$address" >"$run_dir/monitor.out" 2>"$run_dir/monitor.err" &
    pids+=("$!")
fi

echo "running for $duration seconds; dir=$run_dir"
sleep "$duration"

"$monitor" --address "$address" --once || true
sleep 2
echo "run dir: $run_dir"
find "$run_dir" -maxdepth 1 -name '*.mcap' -printf '%f\n'
