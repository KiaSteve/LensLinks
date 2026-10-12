<#
  uninstall.ps1 - removes the LensLinks virtual camera from this PC.
  Run from an elevated PowerShell:
      powershell -ExecutionPolicy Bypass -File .\installer\uninstall.ps1
  Add -KeepData to leave C:\ProgramData\LensLinks (logs) in place.
#>
param(
    [switch]$KeepData
)

$ErrorActionPreference = 'Stop'

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host 'FAILED: run this script from an elevated (Administrator) PowerShell.' -ForegroundColor Red
    exit 1
}

$installDir = 'C:\Program Files\LensLinks'
$dataDir    = 'C:\ProgramData\LensLinks'
$target     = Join-Path $installDir 'LensLinksNative.dll'

Stop-Service -Name FrameServer -Force -ErrorAction SilentlyContinue

if (Test-Path $target) {
    $process = Start-Process regsvr32 -ArgumentList '/u', '/s', "`"$target`"" -Wait -PassThru
    Write-Host "Unregistered (exit code $($process.ExitCode))"
}

if (Test-Path $installDir) {
    Remove-Item $installDir -Recurse -Force -ErrorAction SilentlyContinue
    if (Test-Path $installDir) { Write-Host "Could not remove $installDir (a file is in use)." -ForegroundColor Yellow }
}

if (-not $KeepData -and (Test-Path $dataDir)) {
    Remove-Item $dataDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host 'LensLinks uninstalled.' -ForegroundColor Green
