$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$sourcePath = Join-Path $root 'btstack/at/btstack_at.c'
$headerPath = Join-Path $root 'btstack/include/at/btstack_at.h'
$productionAtDir = Join-Path $root 'btstack/at'

if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw 'btstack/at/btstack_at.c is missing'
}
if (-not (Test-Path -LiteralPath $headerPath)) {
    throw 'btstack/include/at/btstack_at.h is missing'
}

$source = Get-Content -Raw -LiteralPath $sourcePath
$header = Get-Content -Raw -LiteralPath $headerPath
$productionAtSources = (Get-ChildItem -LiteralPath $productionAtDir -File |
    Where-Object { $_.Extension -in @('.c', '.h') } |
    ForEach-Object { Get-Content -Raw -LiteralPath $_.FullName }) -join "`n"
$checks = @(
    @{ Name = 'fixed input ring'; Pass = $source -match 'btstack_at_input_ring_storage\s*\[\s*BTSTACK_AT_INPUT_QUEUE_LEN\s*\]' },
    @{ Name = 'single-slot completion ring'; Pass = $source -match 'btstack_at_completion_ring_storage\s*\[\s*1u\s*\]' },
    @{ Name = 'fixed event ring'; Pass = $source -match 'btstack_at_event_ring_storage\s*\[\s*BTSTACK_AT_EVENT_QUEUE_LEN\s*\]' },
    @{ Name = 'ring initialization'; Pass = ([regex]::Matches($source, 'btstack_ring_init\s*\(').Count -eq 3) },
    @{ Name = 'dedicated AT worker validation'; Pass = $source -match 'btstack_platform_worker_validate\s*\(\s*BTSTACK_WORKER_AT' },
    @{ Name = 'dedicated AT worker start'; Pass = $source -match 'btstack_platform_worker_start\s*\(\s*BTSTACK_WORKER_AT' },
    @{ Name = 'ring receive loop'; Pass = $source -match 'btstack_at_ring_pop\s*\(\s*&btstack_at_input_ring' },
    @{ Name = 'nonblocking ring submit'; Pass = $source -match 'btstack_ring_push\s*\(\s*&btstack_at_input_ring' },
    @{ Name = 'submit unlocks before wake'; Pass = $source -match '(?s)btstack_ring_push\s*\(\s*&btstack_at_input_ring.*?btstack_platform_exit_critical\s*\(\s*\).*?btstack_platform_notify\s*\(\s*BTSTACK_WORKER_AT\s*,\s*BTSTACK_WORKER_SIGNAL_WORK' },
    @{ Name = 'idle wait without polling'; Pass = $source -match 'btstack_platform_wait\s*\(\s*BTSTACK_WORKER_AT\s*,\s*BTSTACK_WORKER_SIGNAL_WORK\s*\|\s*BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE\s*,\s*BTSTACK_WORKER_WAIT_FOREVER\s*\)' },
    @{ Name = 'backend registration'; Pass = $source -match 'btstack_at_backend_register' },
    @{ Name = 'command gate registration'; Pass = $source -match 'btstack_at_command_gate_register' },
    @{ Name = 'command gate before backend'; Pass = $source -match 'btstack_at_command_gate\s*\(\s*&command' },
    @{ Name = 'handled command response'; Pass = $source -match 'BTSTACK_AT_GATE_HANDLED' },
    @{ Name = 'rejected command response'; Pass = $source -match 'BTSTACK_AT_GATE_REJECT' },
    @{ Name = 'completion ingress'; Pass = $source -match 'btstack_at_complete' },
    @{ Name = 'event ingress'; Pass = $source -match 'btstack_at_post_event' },
    @{ Name = 'event push in platform critical section'; Pass = $source -match '(?s)btstack_at_post_event.*?btstack_platform_enter_critical\s*\(\s*\).*?btstack_ring_push\s*\(\s*&btstack_at_event_ring' },
    @{ Name = 'event overflow increments exactly once'; Pass = ([regex]::Matches($source, 'btstack_at_event_overflow_count\s*\+\+').Count -eq 1) -and $source -match '(?s)if\s*\(\s*!pushed\s*\)\s*\{\s*btstack_at_event_overflow_count\s*\+\+\s*;' },
    @{ Name = 'event full path returns without wake'; Pass = $source -match '(?s)btstack_platform_exit_critical\s*\(\s*\)\s*;\s*if\s*\(\s*!pushed\s*\)\s*\{\s*return\s+BTSTACK_AT_STATUS_QUEUE_FULL\s*;\s*\}\s*\(void\)btstack_platform_notify' },
    @{ Name = 'event unlocks before work wake'; Pass = $source -match '(?s)btstack_ring_push\s*\(\s*&btstack_at_event_ring.*?btstack_platform_exit_critical\s*\(\s*\).*?btstack_platform_notify\s*\(\s*BTSTACK_WORKER_AT\s*,\s*BTSTACK_WORKER_SIGNAL_WORK' },
    @{ Name = 'event rejection recorded before ring unlock'; Pass = $source -match '(?s)btstack_ring_push\s*\(\s*&btstack_at_event_ring.*?if\s*\(\s*!pushed\s*\).*?btstack_at_event_rejected\s*\(\s*event\s*\).*?btstack_platform_exit_critical\s*\(\s*\)' },
    @{ Name = 'asynchronous event drain'; Pass = $source -match 'btstack_at_drain_events' },
    @{ Name = 'bounded event drain snapshot'; Pass = $source -match '(?s)pending\s*=\s*btstack_ring_count\s*\(\s*&btstack_at_event_ring\s*\).*?for\s*\(\s*index\s*=\s*0u\s*;\s*index\s*<\s*pending\s*;' },
    @{ Name = 'serialized LOST event drain gate'; Pass = $source -match '(?s)for\s*\(\s*index\s*=\s*0u\s*;\s*index\s*<\s*pending\s*;.*?btstack_at_event_drain_ready\s*\(\s*\).*?break\s*;' },
    @{ Name = 'no unbounded event drain'; Pass = $source -notmatch 'while\s*\(\s*btstack_at_ring_pop\s*\(\s*&btstack_at_event_ring' },
    @{ Name = 'event consumption follows formatting'; Pass = $source -match '(?s)btstack_at_emit_event\s*\(\s*&event\s*\)\s*;\s*btstack_at_event_consumed\s*\(\s*&event\s*\)' },
    @{ Name = 'command result precedes accumulated events'; Pass = $source -match '(?s)btstack_at_emit_result\s*\(\s*&command\s*,\s*&result\s*\)\s*;.*?btstack_at_drain_events\s*\(\s*\)' },
    @{ Name = 'state event deduplication'; Pass = $source -match 'btstack_at_event_is_state' -and $source -match 'btstack_at_last_state_event' },
    @{ Name = 'portable public priority type'; Pass = $header -match 'uint32_t\s+task_priority\s*;' },
    @{ Name = 'no direct RTOS dependency in production AT'; Pass = $productionAtSources -notmatch '(?i)FreeRTOS|queue\.h|task\.h|\bxQueue|\bxTask|\bpdMS|\bTickType|\bportMAX' },
    @{ Name = 'no AT polling interval'; Pass = $productionAtSources -notmatch '(?i)POLL(?:ING)?_MS|20\s*ms|pdMS_TO_TICKS\s*\(\s*20' },
    @{ Name = 'no heap allocation'; Pass = ($source + $header) -notmatch 'pvPortMalloc|malloc\s*\(' }
)

$failed = @($checks | Where-Object { -not $_.Pass })
if ($failed.Count -ne 0) {
    foreach ($check in $failed) {
        Write-Error "Missing: $($check.Name)"
    }
    exit 1
}

$callbackCalls = [regex]::Matches(
    $source,
    'btstack_at_config\.output_callback\s*\(').Count
if ($callbackCalls -ne 1) {
    throw "output callback call sites=$callbackCalls, expected 1"
}

Write-Host 'BTstack AT worker ownership and fixed-ring resource contract is present'
