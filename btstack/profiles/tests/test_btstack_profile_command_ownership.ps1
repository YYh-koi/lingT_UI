$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$hfpHeader = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/include/profiles/btstack_hfp_hf.h')
$hfpSource = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/profiles/btstack_hfp_hf.c')
$avrcpHeader = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/include/profiles/btstack_avrcp.h')
$avrcpSource = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/profiles/btstack_avrcp.c')
$dispatcher = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/core/btstack_core_dispatcher.c')

if ($hfpHeader -notmatch '\bbtstack_hfp_hf_execute_control\s*\(') {
    throw 'HFP header is missing the profile-owned high-level control entry'
}
if ($hfpSource -notmatch '(?s)btstack_hfp_hf_execute_control.*?btstack_hfp_hf_is_audio_connected.*?BTSTACK_HFP_HF_CONTROL_CONNECT_AUDIO') {
    throw 'HFP profile must own toggle/set-audio policy'
}
if ($avrcpHeader -notmatch '\bbtstack_avrcp_execute_control\s*\(') {
    throw 'AVRCP header is missing the profile-owned high-level control entry'
}
if ($avrcpSource -notmatch '(?s)btstack_avrcp_execute_control.*?btstack_avrcp_get_play_status.*?BTSTACK_AVRCP_CONTROL_PLAY.*?BTSTACK_AVRCP_CONTROL_PAUSE') {
    throw 'AVRCP profile must own play/pause policy'
}

foreach ($token in @(
        'BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO',
        'BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE',
        'btstack_core_hfp_control_prepared',
        'btstack_core_avrcp_control_prepared')) {
    if ($dispatcher -match [regex]::Escape($token)) {
        throw "Dispatcher still owns profile command policy: $token"
    }
}

Write-Host 'HFP and AVRCP own their high-level command policy'
