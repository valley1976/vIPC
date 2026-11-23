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

if (-not (Test-ValleySdk $sdk)) {
    $distRoot = Join-Path $root 'dist'
    if (Test-Path $distRoot) {
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
            $sdk = $preferred.FullName
        }
    }
}

if (-not (Test-ValleySdk $sdk)) {
    $fallback = Join-Path $root 'out\Windows_x86_64_Release'
    if (Test-ValleySdk $fallback) {
        $sdk = $fallback
    }
}

if (-not (Test-ValleySdk $sdk)) {
    throw 'build.ps1: cannot find Valley SDK; set VALLEY_SDK or extract a package under dist/'
}
$sdk = (Resolve-Path $sdk).Path

$buildDir = if ($env:VALLEY_BUILD_DIR) {
    $env:VALLEY_BUILD_DIR
} else {
    Join-Path $root 'build\valley_drive'
}

$prefixPath = $sdk
if ($env:VALLEY_CMAKE_PREFIX_PATH) {
    $prefixPath = $prefixPath + ';' + $env:VALLEY_CMAKE_PREFIX_PATH
}

$cmakeArgs = @(
    '-S', $scriptDir,
    '-B', $buildDir,
    '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_PREFIX_PATH=$prefixPath"
)
if ($env:VALLEY_CONAN_TOOLCHAIN) {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$env:VALLEY_CONAN_TOOLCHAIN"
}
if ($env:VALLEY_ASIO_INCLUDE) {
    $cmakeArgs += "-DVALLEY_ASIO_INCLUDE=$env:VALLEY_ASIO_INCLUDE"
}

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    throw 'build.ps1: cmake configure failed; set VALLEY_CMAKE_PREFIX_PATH to dependency configs or VALLEY_CONAN_TOOLCHAIN'
}
& cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) {
    throw 'build.ps1: cmake build failed'
}

$exeDir = if (Test-Path (Join-Path $buildDir 'Release')) {
    Join-Path $buildDir 'Release'
} else {
    $buildDir
}
$exe = Join-Path $exeDir 'valley_drive.exe'
if (Test-Path $exe) {
    Write-Host "built: $exe"
} else {
    Write-Host "built in: $exeDir"
}
