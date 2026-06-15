param(
    [string]$Output = "release/lteapi-runtime.zip"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$ReleaseDir = Join-Path $Root "release/lteapi-runtime"
$OutputPath = Join-Path $Root $Output

if (Test-Path -LiteralPath $ReleaseDir) {
    Remove-Item -LiteralPath $ReleaseDir -Recurse -Force
}
New-Item -ItemType Directory -Path $ReleaseDir | Out-Null

$AddonDir = Join-Path $ReleaseDir "addons/lteapi"
New-Item -ItemType Directory -Path $AddonDir | Out-Null
Copy-Item -LiteralPath (Join-Path $Root "addons/lteapi/plugin.cfg") -Destination $AddonDir -Force

$BinDir = Join-Path $AddonDir "bin"
New-Item -ItemType Directory -Path $BinDir | Out-Null
$SourceBinDir = Join-Path $Root "addons/lteapi/bin"
$RuntimePatterns = @("*.gdextension", "*.uid", "*.dll")
foreach ($Pattern in $RuntimePatterns) {
    Copy-Item -Path (Join-Path $SourceBinDir $Pattern) -Destination $BinDir -Force
}

$OutputParent = Split-Path -Parent $OutputPath
if (!(Test-Path -LiteralPath $OutputParent)) {
    New-Item -ItemType Directory -Path $OutputParent | Out-Null
}
if (Test-Path -LiteralPath $OutputPath) {
    Remove-Item -LiteralPath $OutputPath -Force
}
Compress-Archive -LiteralPath (Join-Path $ReleaseDir "addons") -DestinationPath $OutputPath
Write-Host "Packed LTEAPI runtime: $OutputPath"
