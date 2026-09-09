$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$types = Get-Content -LiteralPath (Join-Path $root 'include\api\bt_xstack_types.h') -Raw
$service = Get-Content -LiteralPath (Join-Path $root 'include\api\bt_xstack_service.h') -Raw
$core = Get-Content -LiteralPath (Join-Path $root 'api\bt_xstack_api_core.c') -Raw
$backend = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_dispatcher.c') -Raw
$internal = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_internal.h') -Raw

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

Require $types 'typedef\s+struct\s*\{[\s\S]*?\}\s*bt_xstack_diagnostics_t\s*;' `
    'Public API must expose cached control-plane diagnostics'
foreach ($field in @(
        'accepted_requests','rejected_requests','stack_commands',
        'stack_command_failures','command_slots_in_use','command_slots_max',
        'run_loop_ingress_depth','run_loop_ingress_max',
        'run_loop_ingress_full','critical_event_drops',
        'best_effort_event_drops')) {
    Require $types ("uint32_t\s+" + $field + '\s*;') `
        "Missing diagnostics field: $field"
}
Require $service `
    'bt_xstack_result_t\s+bt_xstack_get_diagnostics\s*\(\s*bt_xstack_diagnostics_t\s*\*\s*diagnostics\s*\)' `
    'Service API must declare the synchronous diagnostics Getter'
Require $internal 'btstack_core_dispatcher_get_diagnostics\s*\(' `
    'Core must aggregate backend command-slot diagnostics'
Require $backend 'btstack_core_dispatcher_get_diagnostics\s*\(' `
    'Backend must expose command execution and slot diagnostics'

$getter = [regex]::Match($core,
    '(?s)bt_xstack_result_t\s+bt_xstack_get_diagnostics\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $getter.Success) { throw 'Missing diagnostics Getter implementation' }
foreach ($required in @(
        'btstack_core_dispatcher_get_diagnostics',
        'btstack_runtime_get_diagnostics',
        'bt_xstack_critical_event_drop_count',
        'bt_xstack_best_effort_event_drop_count')) {
    Require $getter.Value $required "Diagnostics Getter missing source: $required"
}
if ($getter.Value -match 'bt_xstack_api_submit|btstack_run_loop_execute') {
    throw 'Diagnostics Getter must read cached counters without scheduling work'
}
Require $core 'bt_xstack_api_accepted_request_count\+\+' `
    'Successful queue admission must be counted'
Require $core 'bt_xstack_api_rejected_request_count\+\+' `
    'Failed queue admission must be counted'
Require $backend 'btstack_core_stack_command_count\+\+' `
    'Run-loop command execution must be counted'
Require $backend 'btstack_core_stack_command_failure_count\+\+' `
    'Rejected stack operations must be counted without public completion'

Write-Host 'xStack exposes synchronous cached control-plane diagnostics'
