param(
    [switch]$Tests,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $RepoRoot
try {
    if ($Clean) {
        scons --clean
        Write-Host "Clean completed."
    }
    if ($Tests) {
        scons tests=yes
        if ($LASTEXITCODE -eq 0) {
            ctest --output-on-failure
        }
    } else {
        scons
    }
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build succeeded."
    } else {
        throw "Build failed."
    }
} finally {
    Pop-Location
}
