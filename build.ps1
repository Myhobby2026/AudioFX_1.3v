# ============================================================================
#  build.ps1 - configure + build AudioFX with CMake / Visual Studio 2026
#      powershell -ExecutionPolicy Bypass -File build.ps1 [-Config Release] [-Tests]
# ============================================================================
param(
    [string]$Config = "Release",
    [switch]$Tests
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

# ---- find cmake: prefer the one bundled inside Visual Studio 2026 ------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$cmake = "cmake"

if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($vsPath) { Write-Host "Using Visual Studio: $vsPath" }

    $found = & $vswhere -latest -products * `
        -find "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if ($found) { $cmake = @($found)[0] }
}
Write-Host "Using CMake: $cmake"

# ---- pick the best generator this cmake actually knows ----------------------
$generator = $null
$help = & $cmake --help | Out-String
if ($help -match [regex]::Escape("Visual Studio 18 2026")) {
    $generator = "Visual Studio 18 2026"
}

Push-Location $root
try {
    if ($generator) {
        Write-Host "Generator: $generator (x64)"
        & $cmake -B build -G $generator -A x64
    } else {
        Write-Warning "This CMake does not know 'Visual Studio 18 2026' - using its default generator."
        & $cmake -B build
    }
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed - see the errors above." }

    & $cmake --build build --config $Config
    if ($LASTEXITCODE -ne 0) { throw "Build failed - see the errors above." }

    New-Item -ItemType Directory -Force -Path dist | Out-Null
    $exe = "build\AudioFX_artefacts\$Config\AudioFX.exe"
    if (Test-Path $exe) {
        Copy-Item $exe dist\AudioFX.exe -Force
        Write-Host ""
        Write-Host "BUILD OK -> $root\dist\AudioFX.exe"
    } else {
        Write-Warning "Expected output not found: $exe (check the artefacts folder manually)."
    }

    if ($Tests) {
        Write-Host ""
        Write-Host "Running DSP core validation..."
        g++ -std=c++17 -O2 "$root\tests\dsp_tests.cpp" -o "$root\build\dsp_tests.exe" -I $root
        & "$root\build\dsp_tests.exe"
    }
} finally {
    Pop-Location
}
