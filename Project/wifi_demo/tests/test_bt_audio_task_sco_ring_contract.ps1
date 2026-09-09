$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw -LiteralPath (
    Join-Path $demoRoot 'bt_audio_task.c')
$header = Get-Content -Raw -LiteralPath (
    Join-Path $demoRoot '..\Include\bt_audio_task.h')

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

foreach ($contract in @(
        'BT_AUDIO_SCO_RING_BYTES\s*=\s*2048',
        'BT_AUDIO_SCO_DMA_BUFFER_BYTES\s*=\s*384',
        'BT_AUDIO_SCO_START_BYTES\s*=\s*768',
        'bt_audio_sco_ring_storage',
        'bt_audio_sco_ring_read',
        'bt_audio_sco_ring_write',
        'bt_audio_sco_ring_count',
        'bt_audio_sco_ring_max_count')) {
    Require $source $contract "Missing dedicated SCO ring contract: $contract"
}

foreach ($api in @(
        'bt_audio_source_t',
        'bt_audio_task_diagnostics_t',
        'bt_audio_task_write_media_pcm',
        'bt_audio_task_write_call_pcm',
        'bt_audio_task_get_diagnostics')) {
    Require $header $api "Missing SCO audio task API: $api"
    Require $source $api "Missing SCO audio task implementation: $api"
}

$stop = [regex]::Match(
    $source,
    '(?s)void\s+bt_audio_task_stop\s*\([^)]*\)\s*\{.*?(?=\nint\s+bt_audio_task_configure)')
if (-not $stop.Success) {
    throw 'Cannot locate bt_audio_task_stop implementation'
}
foreach ($contract in @(
        'bt_audio_pending_source\s*=\s*BT_AUDIO_SOURCE_IDLE',
        'bt_audio_config_generation\+\+')) {
    Require $stop.Value $contract `
        "Stop is not serialized as the newest audio generation: $contract"
}
if ($source -match 'bt_audio_stop_requested') {
    throw 'Stop still uses a side flag that can be overwritten by stale configuration'
}

$taskEntry = [regex]::Match(
    $source,
    '(?s)static\s+void\s+bt_audio_task_entry\s*\([^)]*\)\s*\{.*?(?=\nint\s+bt_audio_task_start)')
if (-not $taskEntry.Success) {
    throw 'Cannot locate bt_audio_task_entry implementation'
}
Require $taskEntry.Value `
    'generation\s*==\s*bt_audio_config_generation[\s\S]{0,500}bt_audio_active_source\s*=\s*source' `
    'Audio task publishes a source without rechecking its generation'

$callWriter = [regex]::Match(
    $source,
    '(?s)void\s+bt_audio_task_write_call_pcm\s*\([^)]*\)\s*\{.*?(?=\n(?:void|int)\s+bt_audio_task_)')
if (-not $callWriter.Success) {
    throw 'Cannot locate bt_audio_task_write_call_pcm implementation'
}
if ($callWriter.Value -match 'xStreamBufferSend') {
    throw 'SCO writer still uses partial-write StreamBuffer semantics'
}
foreach ($contract in @(
        'BT_AUDIO_SOURCE_SCO',
        'discard_bytes\s*=\s*\(discard_bytes\s*\+\s*1u\)\s*&\s*~1u',
        'bt_audio_sco_ring_read\s*=.*?discard_bytes',
        'bt_audio_sco_ring_count\s*-=\s*\(uint16_t\)discard_bytes',
        'bt_audio_sco_overflow_count\+\+',
        'bt_audio_sco_dropped_oldest_bytes\s*\+=\s*discard_bytes',
        'xTaskNotifyGive\s*\(\s*bt_audio_task_handle\s*\)')) {
    Require $callWriter.Value $contract `
        "Missing complete SCO ring write behavior: $contract"
}
Require $callWriter.Value `
    'bt_audio_sco_ring_count\s*&\s*1u' `
    'Odd-tail diagnostics do not reflect the committed SCO ring state'

Require $source `
    'available_bytes\s*&=\s*~1u' `
    'SCO consumer does not preserve 16-bit sample alignment'
Require $source `
    'BT_AUDIO_SCO_START_BYTES' `
    'SCO playback does not use the 48-ms byte threshold'
Require $source `
    'memset\s*\(\s*destination\s*\+\s*received\s*,\s*0\s*,\s*missing\s*\)' `
    'SCO DMA underflow is not replaced with bounded silence'

Write-Output 'Dedicated SCO ring preserves complete 16-bit PCM writes'
