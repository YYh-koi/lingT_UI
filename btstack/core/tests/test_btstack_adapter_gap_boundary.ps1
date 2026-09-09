$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$btstackRoot = Join-Path $root 'btstack'
$adapterHeaderPath = Join-Path $btstackRoot 'include/adapter/btstack_adapter.h'
$adapterSourcePath = Join-Path $btstackRoot 'core/btstack_adapter.c'
$dispatcherPath = Join-Path $btstackRoot 'core/btstack_core_dispatcher.c'
$mapperPath = Join-Path $btstackRoot 'core/btstack_event_mapper.c'

foreach ($path in @($adapterHeaderPath, $adapterSourcePath,
        $dispatcherPath, $mapperPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing adapter/GAP boundary artifact: $path"
    }
}

$adapterHeader = Get-Content -Raw -LiteralPath $adapterHeaderPath
$dispatcher = Get-Content -Raw -LiteralPath $dispatcherPath
$mapper = Get-Content -Raw -LiteralPath $mapperPath

foreach ($token in @(
        'btstack_adapter_set_power',
        'btstack_adapter_set_name',
        'btstack_adapter_set_name_with_address_suffix',
        'btstack_adapter_scan_start',
        'btstack_adapter_disconnect',
        'btstack_adapter_get_state',
        'btstack_adapter_set_event_callback')) {
    if ($adapterHeader -notmatch [regex]::Escape($token)) {
        throw "Adapter facade is missing $token"
    }
}

$customProduction = @(Get-ChildItem -LiteralPath $btstackRoot -Recurse -File |
    Where-Object {
        ($_.Extension -in @('.c', '.h')) -and
        ($_.FullName -notmatch '[\\/]tests[\\/]') -and
        ($_.FullName -notmatch '[\\/]src[\\/]') -and
        ($_.FullName -notmatch '[\\/]gap[\\/]') -and
        ($_.FullName -ne $adapterSourcePath)
    })

foreach ($file in $customProduction) {
    $text = Get-Content -Raw -LiteralPath $file.FullName
    if ($text -match '#\s*include\s+"gap/') {
        throw "Only btstack_adapter may include custom GAP headers: $($file.FullName)"
    }
    if (($file.FullName -match '[\\/](core|profiles|api|at)[\\/]') -and
        ($text -match '#\s*include\s+"gap\.h"')) {
        throw "Only btstack_adapter/GAP may include native gap.h: $($file.FullName)"
    }
}

foreach ($token in @(
        'btstack_core_command_slot_acquire',
        'btstack_platform_post_stack_callback')) {
    if ($dispatcher -notmatch [regex]::Escape($token)) {
        throw "Dispatcher lost scheduling responsibility: $token"
    }
}

foreach ($token in @(
        'btstack_gap_',
        'BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO',
        'BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE',
        'btstack_core_disconnect_all_profiles')) {
    if ($dispatcher -match [regex]::Escape($token)) {
        throw "Dispatcher still owns feature policy: $token"
    }
}

if ($mapper -match '#\s*include\s+"gap/') {
    throw 'Event Mapper must consume neutral adapter events, not GAP internals'
}

Write-Host 'Adapter is the sole common GAP facade and dispatcher is policy-free'
