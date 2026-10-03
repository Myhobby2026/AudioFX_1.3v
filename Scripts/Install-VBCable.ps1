# ============================================================================
#  Install-VBCable.ps1 - download + silently install the VB-Cable virtual driver
#  Run from an ELEVATED PowerShell:
#      powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1
#  Agar download fail ho (firewall etc.) to zip khud download karke:
#      powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1 -ZipPath C:\path\VBCABLE_Driver_Pack45.zip
# ============================================================================
param(
    [string]$ZipPath = ""
)
$ErrorActionPreference = "Stop"

$urls = @(
    "https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack45.zip",
    "https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack43.zip"
)

if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
        ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must run as Administrator (the driver is a kernel component)."
}

$work = Join-Path $env:TEMP "vbcable_install"
New-Item -ItemType Directory -Force -Path $work | Out-Null
$zip = Join-Path $work "vbcable.zip"

$downloaded = $false
if ($ZipPath -and (Test-Path $ZipPath)) {
    $zip = $ZipPath
    $downloaded = $true
} else {
    foreach ($url in $urls) {
        try {
            Write-Host "Downloading $url ..."
            Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
            $downloaded = $true
            break
        } catch {
            Write-Warning "Failed: $url"
        }
    }
}

if (-not $downloaded) {
    Write-Host "Automatic download failed - opening the official page instead."
    Start-Process "https://vb-audio.com/Cable/"
    Write-Error "Download VBCABLE_Driver_PackXX.zip manually and re-run this script with -ZipPath <file>."
}

Expand-Archive -Path $zip -DestinationPath $work -Force

$setup = Get-ChildItem -Path $work -Filter "VBCABLE_Setup*.exe" |
         Sort-Object Name -Descending | Select-Object -First 1
if (-not $setup) { Write-Error "VBCABLE_Setup*.exe not found in the archive." }

Write-Host "Installing VB-Cable silently ($($setup.Name)) ..."
Start-Process -FilePath $setup.FullName -ArgumentList "-i" -Verb RunAs -Wait

Write-Host ""
Write-Host "VB-Cable installed. A REBOOT is required before the cable appears."
Write-Host "After reboot: run Scripts\Set-DefaultAudioDevice.ps1 to route all audio through the cable."
