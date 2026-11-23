# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent (Split-Path -Parent $scriptDir)
$sdk = $env:VALLEY_SDK

function Test-ValleySdk([string]$Path) {
    if (-not $Path) {
        return $false
    }
    return Test-Path (Join-Path $Path 'lib/cmake/ipc/ipcConfig.cmake')
}

if (-not (Test-ValleySdk $sdk)) {
    foreach ($candidate in @((Join-Path $root 'sdk'), $root)) {
        if (Test-ValleySdk $candidate) {
            $sdk = $candidate
            break
        }
    }
}

# 仓库里的 dist/ 存放各平台 SDK，例如：
#   dist/valley-1.0.0-windows-x86_64
#   dist/valley-1.0.0-linux-x86_64
if (-not (Test-ValleySdk $sdk)) {
    $distRoot = Join-Path $root 'dist'
    if (Test-Path $distRoot) {
        $platform = if ($env:OS -eq 'Windows_NT') { 'windows' } else { 'linux' }
        $arch = if ($env:PROCESSOR_ARCHITECTURE -match 'ARM64') {
            'aarch64'
        } else {
            'x86_64'
        }
        $candidates = Get-ChildItem -Path $distRoot -Directory `
                -Filter "valley-*-$platform-*" |
            Where-Object {
                Test-ValleySdk $_.FullName
            } |
            Sort-Object Name -Descending
        $preferred = $candidates |
            Where-Object { $_.Name -match "-$arch$" } |
            Select-Object -First 1
        if (-not $preferred) {
            $preferred = $candidates | Select-Object -First 1
        }
        if ($preferred) {
            $sdk = $preferred.FullName
        }
    }
}

if (-not (Test-ValleySdk $sdk)) {
    $outRoot = Join-Path $root 'out'
    if (Test-Path $outRoot) {
        $candidates = Get-ChildItem -Path $outRoot -Directory -Filter '*_Release' |
            Where-Object { Test-ValleySdk $_.FullName }
        $preferred = $candidates | Select-Object -First 1
        if ($preferred) {
            $sdk = $preferred.FullName
        }
    }
}

if (-not (Test-ValleySdk $sdk)) {
    throw 'build.ps1: cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/'
}
$sdk = (Resolve-Path $sdk).Path

$buildDir = if ($env:VALLEY_BUILD_DIR) {
    $env:VALLEY_BUILD_DIR
} else {
    Join-Path $scriptDir 'build-windows'
}

cmake -S $scriptDir -B $buildDir `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$sdk"
cmake --build $buildDir --config Release

$env:PATH = (Join-Path $sdk 'bin') + ';' + $env:PATH
$exeDir = if (Test-Path (Join-Path $buildDir 'Release')) {
    Join-Path $buildDir 'Release'
} else {
    $buildDir
}

foreach ($name in @('minimal_channel', 'minimal_event',
                    'minimal_synchronized')) {
    & (Join-Path $exeDir "$name.exe")
    if ($LASTEXITCODE -ne 0) {
        throw "$name failed with exit code $LASTEXITCODE"
    }
}

$serveExe = Join-Path $exeDir 'minimal_serve.exe'
if (Test-Path $serveExe) {
    & $serveExe
    if ($LASTEXITCODE -ne 0) {
        throw "minimal_serve failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Host 'skip minimal_serve (serve package not found)'
}
