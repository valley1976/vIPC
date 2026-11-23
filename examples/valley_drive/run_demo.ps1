# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

param(
    [string]$ReleaseDir = "",
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

function Test-ValleySdk([string]$Path) {
    if (-not $Path) {
        return $false
    }
    return Test-Path (Join-Path $Path 'lib/cmake/ipc/ipcConfig.cmake')
}

function Find-DistSdk([string]$Root) {
    $distRoot = Join-Path $Root 'dist'
    if (-not (Test-Path $distRoot)) {
        return $null
    }
    $arch = if ($env:PROCESSOR_ARCHITECTURE -match 'ARM64') {
        'aarch64'
    } else {
        'x86_64'
    }
    $candidates = Get-ChildItem -Path $distRoot -Directory `
            -Filter 'valley-*-windows-*' |
        Where-Object { Test-ValleySdk $_.FullName } |
        Sort-Object Name -Descending
    $preferred = $candidates |
        Where-Object { $_.Name -match "-$arch$" } |
        Select-Object -First 1
    if (-not $preferred) {
        $preferred = $candidates | Select-Object -First 1
    }
    if ($preferred) {
        return $preferred.FullName
    }
    return $null
}

# 1. 找 SDK：优先 VALLEY_SDK，其次仓库 dist/ 下的 Windows SDK。
$sdk = $env:VALLEY_SDK
if (-not (Test-ValleySdk $sdk)) {
    $sdk = Find-DistSdk $root
}

# 2. 找运行目录：显式 -ReleaseDir > dist SDK > 仓库根 package 布局 > 旧 out 布局。
if ($ReleaseDir) {
    $release = if ([System.IO.Path]::IsPathRooted($ReleaseDir)) {
        $ReleaseDir
    } else {
        Join-Path $root $ReleaseDir
    }
} elseif (Test-ValleySdk $sdk) {
    $release = $sdk
} elseif (Test-Path (Join-Path $root "bin\supervisor.exe")) {
    $release = $root
} else {
    $fallback = Join-Path $root "out\Windows_x86_64_Release"
    if (Test-Path (Join-Path $fallback "bin\supervisor.exe")) {
        $release = $fallback
    } else {
        throw "cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/"
    }
}
if (-not (Test-Path $release)) {
    throw "release directory not found: $release"
}
$release = (Resolve-Path $release).Path
$bin_dir = Join-Path $release "bin"
if (-not (Test-Path $bin_dir)) {
    throw "missing bin directory in release: $bin_dir"
}

# 3. 找示例可执行文件。SDK 包通常不带 valley_drive.exe，需要单独 build。
$exe = if ($ExampleExe) {
    if ([System.IO.Path]::IsPathRooted($ExampleExe)) {
        $ExampleExe
    } else {
        Join-Path $root $ExampleExe
    }
} else {
    $candidates = @(
        (Join-Path $bin_dir "valley_drive.exe"),
        (Join-Path $root "build\valley_drive_win\Release\valley_drive.exe"),
        (Join-Path $root "build\valley_drive\Release\valley_drive.exe"),
        (Join-Path $root "build\valley_drive\valley_drive.exe"),
        (Join-Path $root "examples\valley_drive\build\Release\valley_drive.exe")
    )
    $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $exe -or -not (Test-Path $exe)) {
    throw "missing valley_drive.exe; build examples/valley_drive or pass -ExampleExe"
}
$exe = (Resolve-Path $exe).Path
Write-Host "example: $exe"
Write-Host "sdk: $release"

$supervisor = Join-Path $bin_dir "supervisor.exe"
$monitor = Join-Path $bin_dir "monitor.exe"
foreach ($path in @($exe, $supervisor)) {
    if (-not (Test-Path $path)) {
        throw "missing executable: $path"
    }
}

# DLL 从 SDK 的 bin 加载；不要修改 SDK 目录本身。
$env:PATH = $bin_dir + ';' + $env:PATH

$run_base = (Get-Location).Path
$run_dir = Join-Path $run_base ("valley_drive_" + $PID)
New-Item -ItemType Directory -Force -Path $run_dir | Out-Null
$conf_dir = Join-Path $run_dir "conf"
New-Item -ItemType Directory -Force -Path $conf_dir | Out-Null
$topology = Join-Path $conf_dir "topology.json"
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "topology.json") `
    -Destination $topology -Force
Write-Host "config: $topology"

$application = [string](Get-Content -LiteralPath $topology -Raw -Encoding UTF8 |
    ConvertFrom-Json).application
if (-not $application) {
    throw "topology.json is missing application"
}
Write-Host "application: $application"

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
        -ArgumentList @("subscriber", "--application", $application,
                        "--duration", "$Duration") `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "subscriber.out") `
        -RedirectStandardError (Join-Path $run_dir "subscriber.err")

    Write-Host "==> recorder"
    $processes += Start-Process -FilePath $exe `
        -ArgumentList @("recorder", "--application", $application,
                        "--duration", "$Duration", "--trigger-after", "5",
                        "--storage", $run_dir) `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "recorder.out") `
        -RedirectStandardError (Join-Path $run_dir "recorder.err")

    Write-Host "==> publisher"
    $processes += Start-Process -FilePath $exe `
        -ArgumentList @("publisher", "--application", $application,
                        "--duration", "$Duration") `
        -WorkingDirectory $run_dir -PassThru `
        -RedirectStandardOutput (Join-Path $run_dir "publisher.out") `
        -RedirectStandardError (Join-Path $run_dir "publisher.err")

    if (-not $Headless -and (Test-Path $monitor)) {
        Write-Host "==> monitor"
        $monitor_start = Get-Date
        $cmd = 'start "" "' + $monitor + '" --application "' + $application + '"'
        & cmd.exe /c $cmd
    }

    Write-Host "running for $Duration seconds"
    Start-Sleep -Seconds $Duration

    if (Test-Path $monitor) {
        Write-Host "==> monitor snapshot"
        & $monitor --application $application --once
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
