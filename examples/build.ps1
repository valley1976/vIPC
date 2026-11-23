# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

param(
    [switch]$MinimalOnly,
    [switch]$ValleyDriveOnly
)

$ErrorActionPreference = 'Stop'
$examplesDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $ValleyDriveOnly) {
    & (Join-Path $examplesDir 'minimal\build.ps1')
    if ($LASTEXITCODE -ne 0) {
        throw "minimal examples failed with exit code $LASTEXITCODE"
    }
}

if (-not $MinimalOnly) {
    & (Join-Path $examplesDir 'valley_drive\build.ps1')
    if ($LASTEXITCODE -ne 0) {
        throw "valley_drive failed with exit code $LASTEXITCODE"
    }
}
