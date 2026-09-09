$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$required = @(
    'btstack/core/btstack_profile_manager.c',
    'btstack/profiles/btstack_avrcp.c',
    'btstack/include/core/btstack_profile_manager.h',
    'btstack/include/profiles/btstack_avrcp.h',
    'btstack/at/btstack_at_core_adapter.c'
)

foreach ($removed in @(
        'btstack/profiles/btstack_classic_profiles.c',
        'btstack/include/profiles/btstack_classic_profiles.h')) {
    if (Test-Path -LiteralPath (Join-Path $root $removed)) {
        $failures += "obsolete Classic composition remains: $removed"
    }
}

$failures = @()
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative))) {
        $failures += "missing $relative"
    }
}

$portFiles = Get-ChildItem -LiteralPath (
    Join-Path $root 'btstack/ports/gpcv2178') -Filter '*.c' |
    Where-Object { $_.Name -ne 'btstack_hci_dump_gpcv2178.c' }
$nativePbapPattern = '\bpbap_'
if (('pbap_connect' -notmatch $nativePbapPattern) -or
    ('btstack_pbap_client_sync' -match $nativePbapPattern)) {
    throw 'Native PBAP boundary pattern is not scoped correctly'
}
$forbiddenPortPatterns = @(
    '\bhfp_hf_',
    '\ba2dp_sink_',
    '\bavrcp_controller_[A-Za-z0-9_]*\s*\(',
    $nativePbapPattern,
    '\bhci_power_control\s*\(',
    'HCI_EVENT_HFP_META',
    'HCI_EVENT_A2DP_META',
    'HCI_EVENT_AVRCP_META',
    'HCI_EVENT_PBAP_META',
    'HFP_SUBEVENT_',
    'A2DP_SUBEVENT_',
    'AVRCP_SUBEVENT_',
    'PBAP_SUBEVENT_',
    'GAP_EVENT_'
)
foreach ($file in $portFiles) {
    $source = Get-Content -Raw -LiteralPath $file.FullName
    foreach ($pattern in $forbiddenPortPatterns) {
        if ($source -match $pattern) {
            $failures += "port contains generic Profile logic: $($file.Name) / $pattern"
        }
    }
}

$backendPath = Join-Path $root 'btstack/at/btstack_at_core_adapter.c'
if (Test-Path -LiteralPath $backendPath) {
    $backend = Get-Content -Raw -LiteralPath $backendPath
    $forbiddenBackendPatterns = @(
        '\bhfp_hf_',
        '\ba2dp_sink_(establish_stream|disconnect)',
        '\bavrcp_controller_',
        'HCI_EVENT_HFP_META',
        'HCI_EVENT_A2DP_META',
        'HCI_EVENT_AVRCP_META'
    )
    foreach ($pattern in $forbiddenBackendPatterns) {
        if ($backend -match $pattern) {
            $failures += "AT backend calls raw Profile API: $pattern"
        }
    }
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Host "FAIL: $_" }
    exit 1
}

Write-Host 'BTstack generic Profile layer boundary is clean'
