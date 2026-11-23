# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$sdk = $env:VALLEY_SDK
if (-not $sdk) {
    throw 'verify.ps1: set VALLEY_SDK to an extracted release package'
}
if (-not (Test-Path (Join-Path $sdk 'lib/cmake/ipc/ipcConfig.cmake'))) {
    throw "verify.ps1: not a Valley SDK: $sdk"
}

$env:VALLEY_SDK = $sdk
& (Join-Path $root 'examples\minimal\build.ps1')
& (Join-Path $root 'benchmarks\build.ps1')
Write-Host 'verify: OK'
