$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$core = Get-Content -LiteralPath (Join-Path $root 'api\bt_xstack_api_core.c') -Raw
$internal = Get-Content -LiteralPath (Join-Path $root 'core\btstack_core_internal.h') -Raw

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}
function Forbid([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) { throw $Message }
}

Require $internal 'BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN\s+16u' `
    'Critical control events need a dedicated fixed queue'
Require $internal 'BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN\s+16u' `
    'Scan results need a dedicated best-effort queue'
Require $internal 'BT_XSTACK_EVENT_TASK_STACK_WORDS\s+1024u' `
    'Event dispatcher must use a fixed static task stack'

foreach ($name in @(
        'bt_xstack_critical_event_queue',
        'bt_xstack_best_effort_event_queue',
        'bt_xstack_critical_event_drop_count',
        'bt_xstack_best_effort_event_drop_count')) {
    Require $core $name "Missing event-dispatch state: $name"
}
Require $core 'static\s+void\s+bt_xstack_event_task\s*\(' `
    'Control callbacks need an independent event task'
Require $core 'btstack_platform_notify\s*\(\s*BTSTACK_WORKER_EVENT\s*,\s*BTSTACK_WORKER_SIGNAL_WORK\s*\)' `
    'Posting an event must immediately wake the event worker'
Require $core 'btstack_platform_wait\s*\(\s*BTSTACK_WORKER_EVENT' `
    'Event worker must block through platform ops instead of polling'
Require $core `
    'BT_XSTACK_SERVICE_EVENT_SCAN_RESULT[\s\S]*?bt_xstack_best_effort_event_queue' `
    'Only scan results must use the best-effort event queue'
Require $core `
    'btstack_ring_pop\s*\(\s*&bt_xstack_critical_event_queue[\s\S]*?btstack_ring_pop\s*\(\s*&bt_xstack_best_effort_event_queue' `
    'Event task must drain critical events before best-effort scan results'
Require $core 'btstack_ring_t\s+bt_xstack_critical_event_queue\s*;' `
    'Critical events must use the common fixed ring'
Require $core 'btstack_ring_t\s+bt_xstack_best_effort_event_queue\s*;' `
    'Best-effort events must use the common fixed ring'

Forbid ($core + "`n" + $internal) 'BT_XSTACK_API_EVENT_POLL_MS' `
    'Fixed 20 ms event polling must be removed'
Forbid $core 'bt_xstack_event_queue\b' `
    'The old shared control-event queue must be removed'
Forbid $core '(?i)\b(?:StaticQueue_t|QueueHandle_t|xQueue\w*|uxQueue\w*)\b' `
    'Event dispatch must not depend on FreeRTOS queues'

Write-Host 'xStack control events use an independent notification-driven dispatcher'
