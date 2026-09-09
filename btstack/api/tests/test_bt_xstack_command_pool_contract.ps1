$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$backend = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_dispatcher.c') -Raw
$core = Get-Content -LiteralPath (Join-Path $root 'api\bt_xstack_api_core.c') -Raw
$internal = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_internal.h') -Raw

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}
function Forbid([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) { throw $Message }
}

Require $internal 'BT_XSTACK_COMMAND_SLOT_COUNT\s+BT_XSTACK_REQUEST_QUEUE_LEN' `
    'Command slot count must match request queue length'
Require $internal 'BT_XSTACK_BACKEND_BUSY\s+1' `
    'Backend must expose a retryable busy result'
Require $backend 'typedef\s+struct\s*\{[\s\S]*?btstack_core_request_t\s+request;[\s\S]*?btstack_context_callback_registration_t\s+callback;[\s\S]*?uint8_t\s+state;[\s\S]*?\}\s*btstack_core_command_slot_t' `
    'Backend must use immutable per-command slots'
Require $backend 'btstack_core_command_slots\s*\[\s*BT_XSTACK_COMMAND_SLOT_COUNT\s*\]' `
    'Backend must allocate the fixed command-slot pool'
Require $backend 'btstack_core_command_slot_release\s*\(' `
    'Run-loop callback must release its command slot'
Require $backend 'btstack_core_command_slot_released\s*\(' `
    'Slot release must notify the API command task'
Require $backend 'if\s*\(\s*!\s*btstack_platform_post_stack_callback\s*\(&slot->callback\)\s*\)' `
    'Backend must use checked run-loop ingress'
Require $core 'btstack_platform_wait\s*\(\s*BTSTACK_WORKER_API\s*,\s*BTSTACK_WORKER_SIGNAL_COMMAND_SLOT\s*\|\s*BTSTACK_WORKER_SIGNAL_WORK\s*,\s*BTSTACK_WORKER_WAIT_FOREVER\s*\)' `
    'Command task must wait only when all command slots are occupied'
Require $core 'BT_XSTACK_BACKEND_BUSY' `
    'Command task must retry retryable backend busy results'

foreach ($obsolete in @(
        'btstack_core_pending_request',
        'bt_xstack_completion_queue',
        'btstack_core_complete',
        'BT_XSTACK_EXEC_TIMEOUT_MS')) {
    Forbid ($backend + "`n" + $core + "`n" + $internal) $obsolete `
        "Obsolete serialized-completion state remains: $obsolete"
}

$task = [regex]::Match($core,
    '(?s)static\s+void\s+bt_xstack_api_task\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $task.Success) { throw 'Missing API command task' }
Forbid $task.Value 'bt_xstack_api_mark_failed' `
    'An individual command failure must not permanently fail the API service'

Write-Host 'xStack API uses a recoverable fixed prepared-command slot pool'
