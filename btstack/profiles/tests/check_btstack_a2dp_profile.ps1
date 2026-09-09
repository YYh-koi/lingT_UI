$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$source = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/profiles/btstack_a2dp_sink.c')
$required = @(
    'a2dp_sink_init',
    'a2dp_sink_create_stream_endpoint',
    'a2dp_sink_register_packet_handler',
    'a2dp_sink_register_media_handler',
    'a2dp_sink_establish_stream',
    'a2dp_sink_disconnect',
    'A2DP_SUBEVENT_SIGNALING_CONNECTION_ESTABLISHED',
    'A2DP_SUBEVENT_STREAM_STARTED',
    'A2DP_SUBEVENT_SIGNALING_CONNECTION_RELEASED'
)
$missing = @($required | Where-Object { $source -notmatch [regex]::Escape($_) })
if ($missing.Count -ne 0) {
    $missing | ForEach-Object { Write-Host "Missing A2DP implementation: $_" }
    exit 1
}
Write-Host 'BTstack generic A2DP Sink Profile implementation is present'
