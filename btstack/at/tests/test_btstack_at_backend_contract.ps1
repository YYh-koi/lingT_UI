$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$adapterPath = Join-Path $root 'btstack/at/btstack_at_core_adapter.c'
$legacyPath = Join-Path $root 'btstack/at/btstack_at_backend.c'

if (-not (Test-Path -LiteralPath $adapterPath -PathType Leaf)) {
    throw 'AT-to-core adapter source is missing'
}
if (Test-Path -LiteralPath $legacyPath) {
    throw 'Legacy independent AT backend must not remain'
}

$source = Get-Content -Raw -LiteralPath $adapterPath
foreach ($required in @(
        'btstack_core_dispatch',
        'btstack_core_copy_state',
        'btstack_core_post_event',
        'btstack_core_request_completed',
        'btstack_at_event_rejected',
        'btstack_at_pbap_space_blocked',
        'btstack_at_core_adapter_init',
        'BT_XSTACK_REQUEST_PBAP_SYNC',
        'BT_XSTACK_REQUEST_PBAP_CANCEL',
        'BT_XSTACK_REQUEST_PBAP_AUTHENTICATE',
        'BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED',
        'BTSTACK_AT_EVENT_HFPLOST',
        'btstack_at_retry_pbap_peer_disconnect')) {
    if ($source -notmatch "\b$required\b") {
        throw "Missing shared-core AT adapter contract: $required"
    }
}

foreach ($forbidden in @(
        'btstack_gap_set_',
        'btstack_hfp_hf_',
        'btstack_a2dp_sink_',
        'btstack_avrcp_control',
        'btstack_pbap_client_peer_disconnected',
        'PBAP_PEER_DISCONNECT_QUEUE_LEN',
        'peer_disconnect_queue',
        'hfp_lost_latched')) {
    if ($source -cmatch "\b$forbidden") {
        throw "AT frontend bypasses the shared core: $forbidden"
    }
}

$slotRelease = [regex]::Match($source,
    '(?s)void\s+btstack_core_command_slot_released\s*\(\s*void\s*\)\s*\{(?<body>.*?)\}')
if (-not $slotRelease.Success) {
    throw 'Missing AT command-slot release hook'
}
$slotReleaseBody = $slotRelease.Groups['body'].Value
foreach ($forbiddenCall in @(
        'btstack_at_retry_pbap_peer_disconnect\s*\(',
        'btstack_core_dispatch\s*\(',
        'btstack_platform_notify\s*\(')) {
    if ($slotReleaseBody -match $forbiddenCall) {
        throw 'AT cleanup must not retry, dispatch, or self-notify on command-slot release'
    }
}
$slotReleaseCode = [regex]::Replace(
    $slotReleaseBody, '(?s)/\*.*?\*/', '')
$slotReleaseCode = [regex]::Replace(
    $slotReleaseCode, '(?m)//.*$', '')
if (-not [string]::IsNullOrWhiteSpace($slotReleaseCode)) {
    throw 'AT command-slot release hook must remain logically empty'
}

Write-Host 'AT frontend delegates commands, state, events, and PCM to shared core'
