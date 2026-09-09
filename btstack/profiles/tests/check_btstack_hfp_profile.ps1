$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$source = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/profiles/btstack_hfp_hf.c')
$required = @(
    'hfp_hf_init',
    'hfp_hf_register_packet_handler',
    'hfp_hf_establish_service_level_connection',
    'hfp_hf_release_service_level_connection',
    'hfp_hf_dial_number',
    'hfp_hf_send_dtmf_code',
    'hfp_hf_answer_incoming_call',
    'hfp_hf_terminate_call',
    'hfp_hf_establish_audio_connection',
    'hfp_hf_activate_voice_recognition',
    'HFP_SUBEVENT_SERVICE_LEVEL_CONNECTION_ESTABLISHED',
    'HFP_SUBEVENT_AG_INDICATOR_STATUS_CHANGED'
)
$missing = @($required | Where-Object { $source -notmatch [regex]::Escape($_) })
if ($missing.Count -ne 0) {
    $missing | ForEach-Object { Write-Host "Missing HFP implementation: $_" }
    exit 1
}
Write-Host 'BTstack generic HFP HF Profile implementation is present'
