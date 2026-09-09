$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'btstack'
$copy = Join-Path $root 'btstack_api'

if (-not (Test-Path -LiteralPath $source)) {
    throw 'Missing original btstack source directory'
}
if (-not (Test-Path -LiteralPath $copy)) {
    throw 'Missing btstack_api source copy'
}

$requiredDirectories = @(
    '3rd-party',
    'adapter',
    'events',
    'gap',
    'ports',
    'profiles',
    'src',
    'transport',
    'api',
    'include\api'
)
foreach ($name in $requiredDirectories) {
    if (-not (Test-Path -LiteralPath (Join-Path $copy $name))) {
        throw "Missing copied stack directory: $name"
    }
}

$sourceRoot = (Resolve-Path -LiteralPath $source).Path
foreach ($file in Get-ChildItem -LiteralPath $sourceRoot -Recurse -File) {
    $relative = $file.FullName.Substring($sourceRoot.Length + 1)
    if (-not (Test-Path -LiteralPath (Join-Path $copy $relative))) {
        throw "Incomplete source copy, missing: $relative"
    }
}

$publicHeader = Join-Path $copy 'include\api\bt_xstack.h'
if (-not (Test-Path -LiteralPath $publicHeader)) {
    throw 'Missing API umbrella header'
}
$publicText = Get-Content -Raw -LiteralPath $publicHeader
if ($publicText -match '\bbt_xstack_send_at\s*\(') {
    throw 'API copy still exports the AT facade'
}

$apiSources = Get-ChildItem -LiteralPath (Join-Path $copy 'api') -File -Filter '*.c'
if ($apiSources.Count -lt 3) {
    throw 'API front end must contain core, backend, and event sources'
}

Write-Output 'xStack API source copy is isolated from the existing AT facade'
