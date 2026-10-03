# ============================================================================
#  Set-DefaultAudioDevice.ps1 - route ALL Windows audio into VB-Cable
#
#  Sets the default playback device to "CABLE Input (VB-Audio Virtual Cable)"
#  so every application's output is captured by AudioFX from "CABLE Output".
#
#      powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1
#      powershell ... -Restore          # put the real speakers/headphones back
# ============================================================================
param(
    [string]$Target = "CABLE Input",
    [switch]$Restore
)

$ErrorActionPreference = "Stop"
Add-Type -Path (Join-Path $PSScriptRoot "WinAudio.cs")

function Show-AllDevices {
    Write-Host ""
    Write-Host "=== Current audio endpoints (PLAY = playback, REC = recording) ==="
    [WinAudio]::ListDevices().Split("`n") | ForEach-Object {
        $p = $_ -split '\|', 4
        if ($p.Count -ge 4) { Write-Host ("  {0,-4} {1,-11} {2}" -f $p[0], $p[1], $p[3]) }
    }
    Write-Host ""
}

if ($Restore) {
    # prefer a real device: speakers / headphones / HDMI
    foreach ($candidate in @("Speakers", "Headphones", "Headset", "HDMI", "Realtek")) {
        $id = [WinAudio]::FindDeviceId($candidate, $true, $false)
        if ($id) { $Target = $candidate; break }
    }
}

Write-Host "Setting default playback device to '*$Target*' (all roles)..."

$activeId = [WinAudio]::FindDeviceId($Target, $true, $false)
if (-not $activeId) {
    $anyId = [WinAudio]::FindDeviceId($Target, $true, $true)
    Show-AllDevices
    if ($anyId) {
        Write-Host "!! '$Target' mila lekin wo DISABLED / NOT-PRESENT state mein hai."
        Write-Host "   Fix: Win+R -> 'mmsys.cpl' -> Playback tab -> '$Target' pe Right-click -> Enable"
        Write-Host "   (agar 'Not present' hai to VB-Cable reinstall karke REBOOT karo)"
    } else {
        Write-Host "!! '$Target' system mein bilkul nahi mila - VB-Cable installed nahi hai."
        Write-Host "   Fix (ADMIN PowerShell mein):"
        Write-Host "     powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1"
        Write-Host "   Phir REBOOT karo, phir ye script dobara chalao."
    }
    exit 1
}

[WinAudio]::SetDefaultDevice($Target, $true, -1)
Write-Host "OK! Default playback ab '*$Target*' hai."
Show-AllDevices
Write-Host "Ab Windows ka saara audio VB-Cable mein jayega."
Write-Host "AudioFX mein Input = 'CABLE Output', Output = apne speakers rakho."
