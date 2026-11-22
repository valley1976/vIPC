# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$sdk = $env:VALLEY_SDK
if (-not $sdk) {
    $candidate = Join-Path $scriptDir '..\..'
    if (Test-Path (Join-Path $candidate 'lib/cmake/ipc/ipcConfig.cmake')) {
        $sdk = (Resolve-Path $candidate).Path
    }
}
if (-not $sdk) {
    $outRoot = Join-Path $scriptDir '..\..\out'
    if (Test-Path $outRoot) {
        $platform = if ($env:OS -eq 'Windows_NT') { 'Windows' } else { 'Linux' }
        $candidates = Get-ChildItem -Path $outRoot -Directory -Filter '*_Release' |
            Where-Object {
                Test-Path (Join-Path $_.FullName 'lib/cmake/ipc/ipcConfig.cmake')
            }
        $preferred = $candidates | Where-Object {
            $_.Name.StartsWith($platform, [System.StringComparison]::OrdinalIgnoreCase)
        } | Select-Object -First 1
        if ($preferred) {
            $sdk = $preferred.FullName
        } elseif ($candidates) {
            $sdk = ($candidates | Select-Object -First 1).FullName
        }
    }
}
if (-not $sdk) {
    throw 'build.ps1: cannot find Valley SDK; set VALLEY_SDK'
}

$buildDir = if ($env:VALLEY_BUILD_DIR) {
    $env:VALLEY_BUILD_DIR
} else {
    Join-Path $scriptDir 'build-windows'
}

cmake -S $scriptDir -B $buildDir `
    -DCMAKE_PREFIX_PATH="$sdk"
cmake --build $buildDir --config Release

$env:PATH = (Join-Path $sdk 'bin') + ';' + $env:PATH
$exeDir = if (Test-Path (Join-Path $buildDir 'Release')) {
    Join-Path $buildDir 'Release'
} else {
    $buildDir
}
& (Join-Path $exeDir 'minimal_channel.exe')
& (Join-Path $exeDir 'minimal_event.exe')
