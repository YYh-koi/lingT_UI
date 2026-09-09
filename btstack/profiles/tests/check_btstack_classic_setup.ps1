$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$source = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/core/btstack_profile_manager.c')
$required = @(
    'sdp_init',
    'rfcomm_init',
    'btstack_a2dp_sink_setup',
    'btstack_avrcp_setup',
    'btstack_hfp_hf_setup',
    'btstack_adapter_set_name',
    'btstack_adapter_set_class_of_device',
    'btstack_adapter_set_connectable',
    'btstack_adapter_set_discoverable'
)
$missing = @($required | Where-Object { $source -notmatch [regex]::Escape($_) })
if ($missing.Count -ne 0) {
    $missing | ForEach-Object { Write-Host "Missing Classic setup: $_" }
    exit 1
}
Write-Host 'BTstack Profile Manager Classic composition is present'
