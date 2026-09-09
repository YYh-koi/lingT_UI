$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$sourcePath = Join-Path $root 'ports\gpcv2178\btstack_run_loop_gpcv2178.c'
$headerPath = Join-Path $root 'ports\gpcv2178\btstack_run_loop_gpcv2178.h'
$source = Get-Content -LiteralPath $sourcePath -Raw
$header = Get-Content -LiteralPath $headerPath -Raw

function Assert-MatchText {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) { throw $Message }
}

Assert-MatchText $source `
    'btstack_context_callback_registration_t\s*\*\s*btstack_run_loop_ingress\s*\[' `
    'Run-loop port must own a fixed callback ingress ring'
Assert-MatchText $source 'BTSTACK_RUN_LOOP_INGRESS_CAPACITY\s+16u' `
    'Run-loop ingress capacity must be fixed at 16 entries'
foreach ($counter in @(
        'btstack_run_loop_ingress_read',
        'btstack_run_loop_ingress_write',
        'btstack_run_loop_ingress_count',
        'btstack_run_loop_ingress_max_depth',
        'btstack_run_loop_ingress_full_count')) {
    Assert-MatchText $source $counter "Missing ingress state: $counter"
}
Assert-MatchText $header `
    'bool\s+btstack_run_loop_gpcv2178_enqueue_callback\s*\(' `
    'Port must expose checked callback enqueue'
Assert-MatchText $header 'btstack_run_loop_gpcv2178_diagnostics_t' `
    'Port must expose ingress diagnostics type'
Assert-MatchText $source 'static\s+void\s+btstack_run_loop_gpcv2178_drain_ingress\s*\(' `
    'Run-loop task must drain the cross-task ingress ring'
Assert-MatchText $source `
    'btstack_run_loop_gpcv2178_drain_ingress\s*\(\s*\)\s*;[\s\S]*?btstack_run_loop_base_execute_callbacks' `
    'Ingress callbacks must drain before base callbacks'
Assert-MatchText $source `
    'taskENTER_CRITICAL\s*\(\s*\)[\s\S]*?taskEXIT_CRITICAL\s*\(\s*\)' `
    'Ingress ring access must use a short FreeRTOS critical section'
Assert-MatchText $source `
    'btstack_run_loop_ingress\s*\[[^\]]+\]\s*==\s*callback_registration' `
    'Ingress enqueue must suppress duplicate callback pointers'
Assert-MatchText $source 'BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE' `
    'Dequeuing stack ingress must expose a distinct AT retry edge'
Assert-MatchText $source `
    '(?s)btstack_run_loop_gpcv2178_dequeue_callback.*?taskEXIT_CRITICAL\s*\(\s*\).*?btstack_platform_notify\s*\(\s*BTSTACK_WORKER_AT\s*,\s*BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE\s*\)' `
    'Run-loop dequeue must notify AT after releasing its critical section'

$execute = [regex]::Match($source,
    '(?s)static\s+void\s+btstack_run_loop_gpcv2178_execute_on_main_thread\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $execute.Success) { throw 'Missing execute_on_main_thread function' }
if ($execute.Value -match 'btstack_run_loop_base_add_callback') {
    throw 'Foreign tasks must not mutate the BTstack base callback list'
}
Assert-MatchText $execute.Value 'btstack_run_loop_gpcv2178_enqueue_callback' `
    'execute_on_main_thread must use the checked port ingress'

Write-Host 'GPCV2178 run loop uses a thread-safe fixed callback ingress ring'
