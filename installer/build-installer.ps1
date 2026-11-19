<#
  build-installer.ps1 - builds LensLinks-Setup-<version>.exe
  Run from the project folder:
      powershell -ExecutionPolicy Bypass -File .\installer\build-installer.ps1
  Needs Inno Setup 6 (https://jrsoftware.org/isinfo.php).
  Add -SkipBuild to reuse the existing Flutter release build.
#>
param([switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $root

function Fail($m) { Write-Host "FAILED: $m" -ForegroundColor Red; exit 1 }

# 1. Find the Inno Setup compiler
$iscc = foreach ($v in '7', '6') {
    "C:\Program Files\Inno Setup $v\ISCC.exe"
    "C:\Program Files (x86)\Inno Setup $v\ISCC.exe"
    "$env:ProgramW6432\Inno Setup $v\ISCC.exe"
    "$env:ProgramFiles\Inno Setup $v\ISCC.exe"
    "${env:ProgramFiles(x86)}\Inno Setup $v\ISCC.exe"
    "$env:LOCALAPPDATA\Programs\Inno Setup $v\ISCC.exe"
}
$iscc = $iscc | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { Fail 'Inno Setup not found. Install it from https://jrsoftware.org/isdl.php and run again.' }
Write-Host "Using $iscc"

# 2. Build the Windows release
if (-not $SkipBuild) {
    Write-Host 'Building Flutter Windows release...'
    flutter build windows --release
    if ($LASTEXITCODE -ne 0) { Fail 'flutter build windows failed.' }
}

# 3. Check the files the installer needs
$release = Join-Path $root 'build\windows\x64\runner\Release'
foreach ($f in 'lenslinks.exe', 'LensLinksNative.dll', 'flutter_windows.dll') {
    if (-not (Test-Path (Join-Path $release $f))) { Fail "Missing $f in $release" }
}

# 4. Compile the installer
& $iscc (Join-Path $PSScriptRoot 'LensLinks.iss')
if ($LASTEXITCODE -ne 0) { Fail 'Inno Setup compile failed.' }

$out = Get-ChildItem (Join-Path $PSScriptRoot 'Output') -Filter 'LensLinks-Setup-*.exe' |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
Write-Host ''
Write-Host "Installer ready: $($out.FullName)" -ForegroundColor Green
Write-Host ("SHA-256: " + (Get-FileHash $out.FullName).Hash)
