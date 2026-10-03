# ============================================================================
#  Register-AudioFX-Autostart.ps1 - start AudioFX with Windows (optional)
# ============================================================================
param(
    [string]$ExePath = "$PSScriptRoot\..\dist\AudioFX.exe",
    [switch]$Remove
)

$runKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"

if ($Remove) {
    Remove-ItemProperty -Path $runKey -Name "AudioFX" -ErrorAction SilentlyContinue
    Write-Host "AudioFX autostart removed."
    return
}

$exe = (Resolve-Path $ExePath).Path
Set-ItemProperty -Path $runKey -Name "AudioFX" -Value "`"$exe`""
Write-Host "AudioFX will now start with Windows: $exe"
