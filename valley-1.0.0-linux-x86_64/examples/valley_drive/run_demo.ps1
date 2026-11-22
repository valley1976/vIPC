# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

param(
    [string]$ReleaseDir = "out\Windows_x86_64_Release",
    [string]$ExampleExe = "",
    [int]$Duration = 30,
    [switch]$Headless,
    [switch]$Keep
)

$ErrorActionPreference = "Stop"

# recorder 在 5s 时触发，后录 5s，因此至少运行 10s。
if ($Duration -lt 10) {
    Write-Warning "Duration < 10s; using 10s so the recorder trigger can finish"
    $Duration = 10
}

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$package_layout = Test-Path (Join-Path $root "bin\supervisor.exe")
$release = if ([System.IO.Path]::IsPathRooted($ReleaseDir)) {
    $ReleaseDir
} elseif ($package_layout) {
    $root
} else {
    Join-Path $root $ReleaseDir
}
$release = (Resolve-Path $release).Path

$exe = if ($ExampleExe) {
    if ([System.IO.Path]::IsPathRooted($ExampleExe)) {
        $ExampleExe
    } else {
        Join-Path $root $ExampleExe
    }
} else {
    $candidates = @(
        (Join-Path $release "bin\valley_drive.exe"),
        (Join-Path $root "build\valley_drive_win\Release\valley_drive.exe"),
        (Join-Path $root "build\valley_drive\Release\valley_drive.exe")
    )
    $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

$bin_dir = Join-Path $release "bin"
New-Item -ItemType Directory -Force -Path $bin_dir | Out-Null
$bin_exe = Join-Path $bin_dir "valley_drive.exe"
if ($exe -and ((Resolve-Path $exe).Path -ne (Join-Path $bin_dir "valley_drive.exe"))) {
    Copy-Item -LiteralPath $exe -Destination $bin_exe -Force
}
if (Test-Path $bin_exe) {
    $exe = $bin_exe
}
Write-Host "example: $exe"

$conf_dir = Join-Path $release "conf"
New-Item -ItemType Directory -Force -Path $conf_dir | Out-Null
$topology = Join-Path $conf_dir "topology.json"
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "topology.json") `
    -Destination $topology -Force
Write-Host "config: $topology"

$supervisor = Join-Path $bin_dir "supervisor.exe"
$monitor = Join-Path $bin_dir "monitor.exe"
foreach ($path in @($exe, $supervisor)) {
    if (-not $path -or -not (Test-Path $path)) {
        throw "missing executable: $path (use -ExampleExe to point to valley_drive.exe)"
    }
}

$run_base = (Get-Location).Path
$run_dir = Join-Path $run_base ("valley_drive_" + $PID)
New-Item -ItemType Directory -Force -Path $run_dir | Out-Null

$application = "valley-drive"
if ($env:OS -eq "Windows_NT") {
    $address = "ipc://vIPC-CHN-$application"
} else {
    $address = "ipc:///tmp/vIPC-CHN-$application.sock"
}

$processes = @()
$monitor_start = $null
try {
    Write-Host "==> supervisor"
    $processes += Start-Process -FilePath $supervisor `
        -ArgumentList @("--config", $topology) `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "supervisor.out") `
        -RedirectStandardError (Join-Path $run_dir "supervisor.err")
    $ready = $false
    for ($i = 0; $i -lt 50; ++$i) {
        Start-Sleep -Milliseconds 100
        if ((Test-Path (Join-Path $run_dir "supervisor.err")) -and
            (Get-Content (Join-Path $run_dir "supervisor.err") -Raw -ErrorAction SilentlyContinue) -match "supervisor endpoint:") {
            $ready = $true
            break
        }
    }
    if (-not $ready) {
        throw "supervisor did not become ready"
    }

    Write-Host "==> subscriber"
    $processes += Start-Process -FilePath $exe `
        -ArgumentList @("subscriber", "--address", $address, "--duration", "$Duration") `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "subscriber.out") `
        -RedirectStandardError (Join-Path $run_dir "subscriber.err")

    Write-Host "==> recorder"
    $processes += Start-Process -FilePath $exe `
        -ArgumentList @("recorder", "--address", $address,
                        "--duration", "$Duration", "--trigger-after", "5",
                        "--storage", $run_dir) `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "recorder.out") `
        -RedirectStandardError (Join-Path $run_dir "recorder.err")

    Write-Host "==> publisher"
    $processes += Start-Process -FilePath $exe `
        -ArgumentList @("publisher", "--address", $address, "--duration", "$Duration") `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "publisher.out") `
        -RedirectStandardError (Join-Path $run_dir "publisher.err")

    if (-not $Headless -and (Test-Path $monitor)) {
        Write-Host "==> monitor"
        $monitor_start = Get-Date
        $cmd = 'start "" "' + $monitor + '" --address "' + $address + '"'
        & cmd.exe /c $cmd
    }

    Write-Host "running for $Duration seconds"
    Start-Sleep -Seconds $Duration

    if (Test-Path $monitor) {
        Write-Host "==> monitor snapshot"
        & $monitor --address $address --once
    }
    Start-Sleep -Seconds 2
} finally {
    if ($monitor_start) {
        Get-Process monitor -ErrorAction SilentlyContinue |
            Where-Object { $_.StartTime -ge $monitor_start } |
            Stop-Process -Force -ErrorAction SilentlyContinue
    }
    foreach ($process in $processes) {
        if ($process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "run dir: $run_dir"
Get-ChildItem $run_dir -Filter "*.mcap" | Select-Object Name, Length
