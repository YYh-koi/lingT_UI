$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$dispatcherPath = Join-Path $root 'btstack/core/btstack_core_dispatcher.c'
$mapperPath = Join-Path $root 'btstack/core/btstack_event_mapper.c'
$internalPath = Join-Path $root 'btstack/core/btstack_core_internal.h'
$managerPath = Join-Path $root 'btstack/core/btstack_profile_manager.c'
$runtimePath = Join-Path $root 'btstack/core/btstack_runtime.c'
$apiPath = Join-Path $root 'btstack/api/bt_xstack_api_core.c'
$atPath = Join-Path $root 'btstack/at/btstack_at_core_adapter.c'

foreach ($path in @($dispatcherPath, $mapperPath, $internalPath, $managerPath,
        $runtimePath,
        $apiPath, $atPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing renamed Core artifact: $path"
    }
}

$dispatcherText = Get-Content -Raw -LiteralPath $dispatcherPath
$runtimeText = Get-Content -Raw -LiteralPath $runtimePath
if ($dispatcherText -match '\bbtstack_core_event_mapper_init\s*\(') {
    throw 'Dispatcher must not own Event Mapper lifecycle'
}
if ($runtimeText -notmatch '\bbtstack_core_event_mapper_init\s*\(') {
    throw 'Runtime must initialize Event Mapper explicitly'
}

foreach ($oldPath in @(
        'btstack/core/btstack_backend.c',
        'btstack/core/btstack_core_backend.c',
        'btstack/core/btstack_core_event_mapper.c')) {
    if (Test-Path -LiteralPath (Join-Path $root $oldPath)) {
        throw "Redundant Core filename remains: $oldPath"
    }
}

$sharedText = @(
    Get-Content -Raw -LiteralPath $dispatcherPath
    Get-Content -Raw -LiteralPath $mapperPath
    Get-Content -Raw -LiteralPath $internalPath
    Get-Content -Raw -LiteralPath $managerPath
) -join "`n"

if ($sharedText -cmatch '\bbt_xstack_api_') {
    throw 'Shared Core production code still exposes API-specific internal names'
}

$boundaryText = $sharedText + "`n" +
    (Get-Content -Raw -LiteralPath $apiPath) + "`n" +
    (Get-Content -Raw -LiteralPath $atPath)

foreach ($token in @(
        'btstack_core_request_t',
        'btstack_core_event_envelope_t',
        'btstack_core_dispatch',
        'btstack_core_copy_state',
        'btstack_core_state_read_hfp',
        'btstack_core_dispatcher_init',
        'btstack_core_dispatcher_get_diagnostics',
        'btstack_event_mapper_ready',
        'btstack_frontend_deliver_event')) {
    if ($boundaryText -notmatch [regex]::Escape($token)) {
        throw "Missing shared Core name: $token"
    }
}

$forbidden = @(
    'bt_xstack_api_request_t',
    'bt_xstack_api_service_snapshot_t',
    'bt_xstack_api_hfp_snapshot_t',
    'bt_xstack_api_a2dp_snapshot_t',
    'bt_xstack_api_avrcp_snapshot_t',
    'bt_xstack_api_backend_diagnostics_t',
    'bt_xstack_api_event_domain_t',
    'bt_xstack_api_event_envelope_t',
    'bt_xstack_api_backend_dispatch',
    'bt_xstack_api_backend_copy_state',
    'bt_xstack_api_backend_get_diagnostics',
    'bt_xstack_api_backend_ready',
    'bt_xstack_api_state_',
    'bt_xstack_api_event_post_microphone_mute',
    'bt_xstack_api_event_set_scan_filter',
    'bt_xstack_api_deliver_event'
    'btstack_core_backend_init'
    'btstack_core_backend_get_diagnostics'
    'btstack_core_backend_ready'
)
foreach ($token in $forbidden) {
    if ($boundaryText -cmatch [regex]::Escape($token)) {
        throw "Obsolete shared API-specific name remains: $token"
    }
}

Write-Host 'Shared Core dispatcher and Event Mapper names match their responsibilities'
