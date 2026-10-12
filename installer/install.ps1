<#
  install.ps1 - installs the LensLinks virtual camera on this PC.
  Run from an elevated PowerShell:
      powershell -ExecutionPolicy Bypass -File .\installer\install.ps1
  Optional: -Dll <path to LensLinksNative.dll>  (default: Release build)
#>
param(
    [string]$Dll = (Join-Path $PSScriptRoot '..\native_build\Release\LensLinksNative.dll')
)

$ErrorActionPreference = 'Stop'

function Fail($message) {
    Write-Host "FAILED: $message" -ForegroundColor Red
    exit 1
}

# 1. Must be elevated
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Fail 'Run this script from an elevated (Administrator) PowerShell.'
}

if (-not (Test-Path $Dll)) { Fail "DLL not found: $Dll" }
$Dll = (Resolve-Path $Dll).Path

$installDir = 'C:\Program Files\LensLinks'
$dataDir    = 'C:\ProgramData\LensLinks'
$target     = Join-Path $installDir 'LensLinksNative.dll'

# 2. Stop Frame Server so the DLL is not locked
Write-Host 'Stopping Windows Camera Frame Server...'
Stop-Service -Name FrameServer -Force -ErrorAction SilentlyContinue
$service = Get-Service -Name FrameServer -ErrorAction SilentlyContinue
if ($service -and $service.Status -ne 'Stopped') {
    Fail 'Could not stop FrameServer. Close any app using a camera (browser, Teams, Zoom, Camera) and run again.'
}

# 3. Unregister the old copy if there is one (ignore errors)
if (Test-Path $target) {
    Start-Process regsvr32 -ArgumentList '/u', '/s', "`"$target`"" -Wait | Out-Null
}

# 4. Copy the DLL
New-Item -ItemType Directory -Force $installDir | Out-Null
try {
    Copy-Item $Dll $target -Force
} catch {
    Fail "Could not copy the DLL to $installDir : $($_.Exception.Message)"
}
$sourceHash = (Get-FileHash $Dll).Hash
$targetHash = (Get-FileHash $target).Hash
if ($sourceHash -ne $targetHash) { Fail 'Copied DLL does not match the source (hash mismatch).' }
Write-Host "Copied DLL ($targetHash)"

# 5. Register
$process = Start-Process regsvr32 -ArgumentList '/s', "`"$target`"" -Wait -PassThru
if ($process.ExitCode -ne 0) {
    Fail "regsvr32 failed with exit code $($process.ExitCode) (5 = access denied, 3 = DLL did not load)."
}
$clsidKey = 'HKLM:\SOFTWARE\Classes\CLSID\{8F8B9C41-4A6D-4C7B-912A-7E529D138461}\InprocServer32'
$registered = (Get-ItemProperty $clsidKey -ErrorAction SilentlyContinue).'(default)'
if ($registered -ne $target) { Fail "Registration check failed. Registry says: $registered" }
Write-Host "Registered: $registered"

# 6. Data folder and permissions
New-Item -ItemType Directory -Force $dataDir | Out-Null

# Reset to explicit permissions: remove inheritance, then grant only what is needed.
#   SYSTEM and Administrators : full control
#   Users                     : modify (the app writes frames and the log)
#   LOCAL SERVICE             : read and execute (Frame Server only reads frames)
& icacls $dataDir /inheritance:r | Out-Null
& icacls $dataDir /remove:g '*S-1-1-0' '*S-1-15-2-1' | Out-Null
& icacls $dataDir /grant:r `
    '*S-1-5-18:(OI)(CI)F' `
    '*S-1-5-32-544:(OI)(CI)F' `
    '*S-1-5-32-545:(OI)(CI)M' `
    '*S-1-5-19:(OI)(CI)RX' | Out-Null
if ($LASTEXITCODE -ne 0) { Fail 'Could not set permissions on the data folder.' }

# Files created earlier with broader permissions: reset them to inherit from the folder.
Get-ChildItem $dataDir -File -ErrorAction SilentlyContinue | ForEach-Object {
    & icacls $_.FullName /reset | Out-Null
}
Write-Host "Permissions set on $dataDir"
& icacls $dataDir

Write-Host ''
Write-Host 'LensLinks installed. Start the app and the camera will appear as "LensLinks Camera".' -ForegroundColor Green

