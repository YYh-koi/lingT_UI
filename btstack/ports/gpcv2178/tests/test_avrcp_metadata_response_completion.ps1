$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$controllerPath = Join-Path $root 'btstack/src/classic/avrcp_controller.c'

if (-not (Test-Path -LiteralPath $controllerPath)) {
    throw "Missing AVRCP Controller source: $controllerPath"
}

$controller = Get-Content -Raw -LiteralPath $controllerPath
$handler = [regex]::Match(
    $controller,
    '(?s)case\s+AVRCP_PDU_ID_GET_ELEMENT_ATTRIBUTES\s*:.*?\n\s*break\s*;')

if (-not $handler.Success) {
    throw 'Missing GetElementAttributes response handler'
}

if ($handler.Value -notmatch 'list_size\s*=\s*\(uint16_t\)\s*\(param_length\s*-\s*1u\)') {
    throw 'GetElementAttributes list size still includes the num_attributes byte'
}

if ($handler.Value -notmatch '(?s)list_size\s*==\s*0u.*?avrcp_controller_emit_now_playing_info_event_done') {
    throw 'Empty GetElementAttributes responses do not emit completion'
}

Write-Host 'AVRCP metadata responses use the payload length and always complete when empty'
