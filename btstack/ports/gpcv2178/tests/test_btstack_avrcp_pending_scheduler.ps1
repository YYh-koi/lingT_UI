$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$profilePath = Join-Path $root 'btstack/profiles/btstack_avrcp_controller.c'
$facadePath = Join-Path $root 'btstack/profiles/btstack_avrcp.c'
$managerPath = Join-Path $root 'btstack/core/btstack_profile_manager.c'

foreach ($path in @($profilePath, $facadePath, $managerPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing AVRCP scheduler artifact: $path"
    }
}

$profile = Get-Content -Raw -LiteralPath $profilePath
$facade = Get-Content -Raw -LiteralPath $facadePath
$manager = Get-Content -Raw -LiteralPath $managerPath

foreach ($token in @(
        'BTSTACK_AVRCP_PENDING_METADATA',
        'BTSTACK_AVRCP_PENDING_PLAY_STATUS',
        'BTSTACK_AVRCP_REQUEST_RETRY_MS',
        'btstack_avrcp_pending_requests',
        'btstack_avrcp_schedule_request',
        'btstack_avrcp_try_pending_request',
        'btstack_avrcp_clear_pending_requests')) {
    if ($profile -notmatch [regex]::Escape($token)) {
        throw "Missing AVRCP pending scheduler token: $token"
    }
}

if ($profile -notmatch '(?s)static\s+void\s+btstack_avrcp_schedule_request\s*\([^)]*\).*?btstack_avrcp_pending_requests\s*\|=\s*request_mask.*?btstack_avrcp_arm_request_retry') {
    throw 'AVRCP requests are not coalesced into the pending bitmask'
}

$service = [regex]::Match(
    $profile,
    '(?s)static\s+void\s+btstack_avrcp_try_pending_request\s*\(\s*void\s*\)\s*\{.*?\n\}')
if (-not $service.Success) {
    throw 'Missing AVRCP pending request service function'
}
$metadataIndex = $service.Value.IndexOf('BTSTACK_AVRCP_PENDING_METADATA')
$playStatusIndex = $service.Value.IndexOf('BTSTACK_AVRCP_PENDING_PLAY_STATUS')
if (($metadataIndex -lt 0) -or ($playStatusIndex -lt 0) -or
    ($metadataIndex -ge $playStatusIndex)) {
    throw 'Metadata is not prioritized ahead of play-status polling'
}
if ($service.Value -notmatch '(?s)status\s*!=\s*ERROR_CODE_COMMAND_DISALLOWED.*?btstack_avrcp_pending_requests\s*&=') {
    throw 'Non-busy AVRCP submissions do not clear their selected pending bit'
}

$progressHandler = [regex]::Match(
    $profile,
    '(?s)static\s+void\s+btstack_avrcp_progress_timeout\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $progressHandler.Success -or
    $progressHandler.Value -notmatch 'btstack_avrcp_schedule_request\s*\(\s*BTSTACK_AVRCP_PENDING_PLAY_STATUS\s*\)') {
    throw 'Periodic play-status polling does not use the pending scheduler'
}
if ($progressHandler.Value -match 'avrcp_controller_get_play_status\s*\(') {
    throw 'Progress timer still submits directly to a busy AVRCP Controller'
}

$metadataRequest = [regex]::Match(
    $facade,
    '(?s)int\s+btstack_avrcp_request_metadata\s*\(\s*void\s*\)\s*\{.*?\n\}')
if (-not $metadataRequest.Success -or
    $metadataRequest.Value -notmatch 'btstack_avrcp_controller_request_metadata\s*\(\s*\)') {
    throw 'Public metadata request does not route through the Controller'
}
if ($profile -notmatch '(?s)int\s+btstack_avrcp_controller_request_metadata\s*\(\s*void\s*\).*?btstack_avrcp_schedule_request\s*\(\s*BTSTACK_AVRCP_PENDING_METADATA\s*\)') {
    throw 'Controller metadata request does not use the pending scheduler'
}
if ($facade -notmatch '(?s)AVRCP_SUBEVENT_CONNECTION_RELEASED:.*?btstack_avrcp_controller_disconnected\s*\(\s*\)') {
    throw 'AVRCP disconnect does not notify the Controller'
}
if ($profile -notmatch '(?s)void\s+btstack_avrcp_controller_disconnected\s*\(\s*void\s*\).*?btstack_avrcp_clear_pending_requests\s*\(\s*\)') {
    throw 'AVRCP Controller disconnect does not clear pending requests and retry timer'
}
if ($manager -notmatch '(?s)BTSTACK_AVRCP_EVENT_TRACK_CHANGED:.*?btstack_avrcp_request_metadata\s*\(\s*\)') {
    throw 'Track changes no longer trigger the fixed metadata policy'
}

Write-Host 'AVRCP pending scheduler coalesces queries, prioritizes metadata, and retries busy submissions'
