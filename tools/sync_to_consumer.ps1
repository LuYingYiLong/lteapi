param(
    [Parameter(Mandatory)]
    [string]$Target,
    [string]$Source = $PSScriptRoot | Split-Path -Parent
)

$ErrorActionPreference = "Stop"
$SourceAddon = Join-Path $Source "addons/lteapi"
$TargetAddon = Join-Path $Target "addons/lteapi"

if (!(Test-Path -LiteralPath $SourceAddon)) {
    throw "LTEAPI addon not found: $SourceAddon"
}

if (!(Test-Path -LiteralPath $Target)) {
    throw "Target project not found: $Target"
}

# Copy plugin.cfg
Copy-Item -LiteralPath (Join-Path $SourceAddon "plugin.cfg") -Destination $TargetAddon -Force

# Copy runtime binaries
$SourceBin = Join-Path $SourceAddon "bin"
$TargetBin = Join-Path $TargetAddon "bin"
if (!(Test-Path -LiteralPath $TargetBin)) {
    New-Item -ItemType Directory -Path $TargetBin -Force | Out-Null
}
foreach ($Pattern in @("*.gdextension", "*.uid", "*.dll")) {
    $Files = Get-ChildItem -Path $SourceBin -Filter $Pattern -ErrorAction SilentlyContinue
    foreach ($File in $Files) {
        Copy-Item -LiteralPath $File.FullName -Destination $TargetBin -Force
    }
}

# Copy tools if they exist
$SourceTools = Join-Path $Source "tools"
$TargetTools = Join-Path $TargetAddon "tools"
if (Test-Path -LiteralPath $SourceTools) {
    if (!(Test-Path -LiteralPath $TargetTools)) {
        New-Item -ItemType Directory -Path $TargetTools -Force | Out-Null
    }
    Copy-Item -Path (Join-Path $SourceTools "lteplugin_*") -Destination $TargetTools -Force
}

Write-Host "Synced LTEAPI runtime to $Target"
