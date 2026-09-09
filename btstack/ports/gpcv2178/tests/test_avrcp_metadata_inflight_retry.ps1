$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$profilePath = Join-Path $root 'btstack/profiles/btstack_avrcp.c'

if (-not (Test-Path -LiteralPath $profilePath)) {
    throw "Missing AVRCP profile source: $profilePath"
}

$profile = Get-Content -Raw -LiteralPath $profilePath

foreach ($token in @(
        'BTSTACK_AVRCP_METADATA_RETRY_MS',
        'BTSTACK_AVRCP_METADATA_MAX_RETRIES',
        'btstack_avrcp_metadata_inflight',
        'btstack_avrcp_metadata_retry_count',
        'btstack_avrcp_metadata_complete')) {
    if ($profile -notmatch [regex]::Escape($token)) {
        throw "Missing AVRCP metadata retry token: $token"
    }
}

$service = [regex]::Match(
    $profile,
    '(?s)static\s+void\s+btstack_avrcp_try_pending_request\s*\(\s*void\s*\).*?(?=static\s+void\s+btstack_avrcp_progress_timeout)')
if (-not $service.Success) {
    throw 'Missing AVRCP pending request service function'
}
if ($service.Value -notmatch '(?s)BTSTACK_AVRCP_PENDING_METADATA.*?btstack_avrcp_metadata_inflight.*?return') {
    throw 'Metadata is submitted again while an earlier request is in flight'
}
if ($service.Value -notmatch '(?s)status\s*==\s*ERROR_CODE_SUCCESS.*?btstack_avrcp_metadata_inflight\s*=\s*true') {
    throw 'Accepted metadata requests are not marked in flight'
}

$complete = [regex]::Match(
    $profile,
    '(?s)static\s+void\s+btstack_avrcp_metadata_complete\s*\([^)]*\).*?(?=static\s+void\s+btstack_avrcp_try_pending_request)')
if (-not $complete.Success) {
    throw 'Missing AVRCP metadata completion handler'
}
if ($complete.Value -notmatch '(?s)status\s*==\s*ERROR_CODE_SUCCESS.*?return\s*;.*?BTSTACK_AVRCP_METADATA_MAX_RETRIES.*?BTSTACK_AVRCP_PENDING_METADATA.*?BTSTACK_AVRCP_METADATA_RETRY_MS') {
    throw 'Failed metadata requests are not retried with a bounded delay'
}

$packetHandler = [regex]::Match(
    $profile,
    '(?s)static\s+void\s+btstack_avrcp_controller_packet_handler\s*\([^)]*\).*?(?=static\s+void\s+btstack_avrcp_target_packet_handler)')
if (-not $packetHandler.Success) {
    throw 'Missing AVRCP Controller packet handler'
}
if ($packetHandler.Value -notmatch '(?s)AVRCP_SUBEVENT_NOW_PLAYING_INFO_DONE.*?btstack_avrcp_metadata_complete') {
    throw 'Metadata DONE events do not finish or retry the in-flight request'
}
if ($packetHandler.Value -notmatch '(?s)AVRCP_SUBEVENT_OPERATION_COMPLETE.*?AVRCP_PDU_ID_GET_ELEMENT_ATTRIBUTES.*?ERROR_CODE_PAGE_TIMEOUT.*?btstack_avrcp_metadata_complete') {
    throw 'GetElementAttributes timeouts do not retry metadata'
}

Write-Host 'AVRCP metadata remains in flight until DONE and retries bounded failures'
