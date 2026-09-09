$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent $PSScriptRoot
$internal = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_core_internal.h')
$backend = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_core_dispatcher.c')
$events = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_event_mapper.c')

if ($events -notmatch '#include\s+"adapter/btstack_adapter\.h"') {
    throw 'Event Mapper must consume GAP state and events through adapter'
}
if ($events -match '#include\s+"gap/') {
    throw 'Event Mapper must not include private GAP headers'
}

if ($internal -notmatch '\bbtstack_core_state_set_value\s*\(') {
    throw 'Missing synchronized state update API'
}
if ($backend -match '\bbtstack_core_state_mutable\s*\(') {
    throw 'Backend must not mutate the shared state through a raw pointer'
}
if ($events -notmatch '(?s)btstack_core_state_sync_gap\s*\([^)]*\).*?btstack_platform_enter_critical\s*\(\).*?btstack_platform_exit_critical\s*\(\)') {
    throw 'GAP snapshot updates must be protected by a critical section'
}
if ($events -notmatch '(?s)btstack_core_state_set_value\s*\([^)]*\).*?btstack_platform_enter_critical\s*\(\).*?btstack_platform_exit_critical\s*\(\)') {
    throw 'Request-driven state updates must be protected by a critical section'
}
if ($events -notmatch '(?s)btstack_core_copy_state\s*\([^)]*\).*?btstack_platform_enter_critical\s*\(\).*?memcpy\s*\(.*?btstack_platform_exit_critical\s*\(\)') {
    throw 'Synchronous Getter snapshots must copy under a critical section'
}

Write-Output 'xStack synchronous Getter state has protected readers and writers'
