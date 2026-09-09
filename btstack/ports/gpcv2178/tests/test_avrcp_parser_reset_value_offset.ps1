$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$controllerPath = Join-Path $root 'btstack/src/classic/avrcp_controller.c'

if (-not (Test-Path -LiteralPath $controllerPath)) {
    throw "Missing AVRCP Controller source: $controllerPath"
}

$controller = Get-Content -Raw -LiteralPath $controllerPath
$parserReset = [regex]::Match(
    $controller,
    '(?s)static\s+void\s+avrcp_parser_reset\s*\([^)]*\)\s*\{.*?\n\}')

if (-not $parserReset.Success) {
    throw 'Missing avrcp_parser_reset implementation'
}

if ($parserReset.Value -notmatch 'connection->attribute_value_offset\s*=\s*0\s*;') {
    throw 'AVRCP parser reset leaves the previous metadata value offset active'
}

Write-Host 'AVRCP parser reset clears the metadata value offset'
