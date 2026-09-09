$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $demoRoot 'bt_audio_task.c'
if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw 'Missing bt_audio_task.c'
}
$source = Get-Content -Raw -LiteralPath $sourcePath

foreach ($required in @(
        '#include "stream_buffer.h"',
        'xStreamBufferCreateStatic',
        'xStreamBufferSend',
        'xStreamBufferReceive',
        'xStreamBufferBytesAvailable',
        'xStreamBufferReset',
        'BT_AUDIO_SCO_DMA_BUFFER_BYTES',
        'BT_AUDIO_SCO_RING_BYTES',
        'BT_AUDIO_SCO_START_BYTES',
        'bt_audio_task_write_call_pcm',
        'BT_AUDIO_PCM_BUFFER_BYTES')) {
    if ($source -notmatch [regex]::Escape($required)) {
        throw "Missing SCO byte-stream behavior: $required"
    }
}

if ($source -match 'BT_AUDIO_PREBUFFER_CHUNKS|uxQueueMessagesWaiting') {
    throw 'Audio prebuffer still depends on packet count'
}
if ($source -notmatch '(?s)sample_rate\s*!=\s*8000u.*sample_rate\s*!=\s*44100u.*sample_rate\s*!=\s*48000u') {
    throw 'Audio task does not accept SCO and A2DP sample rates'
}
if ($source -notmatch 'sample_rate\s*==\s*8000u[\s\S]{0,160}channel_count\s*!=\s*1u') {
    throw '8 kHz SCO configuration is not restricted to mono'
}
if ($source -notmatch 'BT_AUDIO_DIAG_LOG_INTERVAL\s*=\s*32') {
    throw 'Audio diagnostics do not define a rate limit'
}
foreach ($counter in @('bt_audio_drop_count', 'bt_audio_underflow_count')) {
    if ($source -notmatch ([regex]::Escape($counter) +
            '\s*%\s*BT_AUDIO_DIAG_LOG_INTERVAL')) {
        throw "Audio diagnostics are not rate limited: $counter"
    }
}
if ($source -notmatch '(?s)BT_AUDIO_SOURCE_A2DP.*?xStreamBufferBytesAvailable\s*\(.*?dma_buffer_bytes\s*\*\s*2u') {
    throw 'A2DP DAC does not prebuffer both active DMA buffers by byte count'
}
if ($source -notmatch '(?s)BT_AUDIO_SOURCE_SCO.*?BT_AUDIO_SCO_START_BYTES') {
    throw 'SCO DAC does not use its dedicated byte prebuffer threshold'
}

Write-Output 'SCO and A2DP byte-stream contracts are separated'
