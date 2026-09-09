$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$controllerPath = Join-Path $root 'btstack/src/classic/avrcp_controller.c'

if (-not (Test-Path -LiteralPath $controllerPath)) {
    throw "Missing AVRCP Controller source: $controllerPath"
}

$controller = Get-Content -Raw -LiteralPath $controllerPath

$changedHandler = [regex]::Match(
    $controller,
    '(?s)case\s+AVRCP_CTYPE_RESPONSE_CHANGED_STABLE\s*:.*?\n\s*break\s*;')
if (-not $changedHandler.Success) {
    throw 'Missing AVRCP notification changed handler'
}
foreach ($token in @(
        'notification changed event=%u',
        're-register queued event=%u')) {
    if ($changedHandler.Value -notmatch [regex]::Escape($token)) {
        throw "Missing Track Changed diagnostic: $token"
    }
}

$sendHandler = [regex]::Match(
    $controller,
    '(?s)static\s+void\s+avrcp_controller_handle_can_send_now\s*\([^)]*\).*?(?=static\s+void\s+avrcp_controller_packet_handler)')
if (-not $sendHandler.Success) {
    throw 'Missing pending notification send handler'
}
if ($sendHandler.Value -notmatch 'send_status\s*=\s*avrcp_send_register_notification') {
    throw 'Notification registration send status is still ignored'
}
if ($sendHandler.Value -notmatch '(?s)send_status\s*==\s*ERROR_CODE_SUCCESS.*?controller_notifications_to_register\s*&=') {
    throw 'Notification pending bit is cleared before send success'
}
if ($sendHandler.Value -notmatch '(?s)else\s*\{.*?avrcp_request_can_send_now') {
    throw 'Failed notification registration is not retried'
}
if ($sendHandler.Value -notmatch [regex]::Escape('register tx event=%u status=%d')) {
    throw 'Missing notification registration transmit diagnostic'
}

Write-Host 'AVRCP notification registration retains pending state and retries failed sends'
