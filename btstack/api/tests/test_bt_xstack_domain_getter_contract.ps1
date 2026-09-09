$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$core = Get-Content -LiteralPath (Join-Path $root 'api\bt_xstack_api_core.c') -Raw
$events = Get-Content -LiteralPath (Join-Path $root 'core\btstack_event_mapper.c') -Raw
$internal = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_internal.h') -Raw

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

foreach ($type in @(
        'btstack_core_service_snapshot_t',
        'btstack_core_hfp_snapshot_t',
        'btstack_core_a2dp_snapshot_t',
        'btstack_core_avrcp_snapshot_t')) {
    Require $internal $type "Missing compact cached-state domain: $type"
}
foreach ($reader in @(
        'btstack_core_state_read_service',
        'btstack_core_state_read_hfp',
        'btstack_core_state_read_a2dp',
        'btstack_core_state_read_avrcp',
        'btstack_core_state_get_peer_count',
        'btstack_core_state_get_peer')) {
    Require $events ("int\s+" + $reader + "\s*\(") `
        "Missing short cached-state reader: $reader"
    Require $core ($reader + '\s*\(') `
        "Public Getters must use short cached-state reader: $reader"
}

$wholeStateUses = [regex]::Matches($core, 'bt_xstack_api_snapshot\s*\(').Count
if ($wholeStateUses -ne 2) {
    throw "Only the snapshot helper and bt_xstack_get_state may use the whole-state path; found $wholeStateUses references"
}

foreach ($getter in @(
        'bt_xstack_get_hfp_state',
        'bt_xstack_get_microphone_mute',
        'bt_xstack_get_a2dp_state',
        'bt_xstack_get_avrcp_state',
        'bt_xstack_get_avrcp_play_state')) {
    $match = [regex]::Match($core,
        'bt_xstack_result_t\s+' + $getter + '\s*\([^)]*\)\s*\{[^}]*\}')
    if (-not $match.Success) { throw "Missing Getter: $getter" }
    if ($match.Value -match 'bt_xstack_api_snapshot') {
        throw "$getter must not copy the complete bt_xstack_state_t"
    }
    if ($match.Value -match 'bt_xstack_api_submit') {
        throw "$getter must not enter the request or run-loop path"
    }
}

Write-Host 'xStack synchronous Getters use compact cached-state domains'
