$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (
    Split-Path -Parent (
        Split-Path -Parent $PSScriptRoot))

$required = @(
    'btstack/include/gap/btstack_gap.h',
    'btstack/gap/btstack_gap.c',
    'btstack/gap/btstack_gap_classic.c',
    'btstack/gap/btstack_gap_le.c',
    'btstack/gap/btstack_gap_ad_data.c'
)

$forbidden = @(
    'btstack/gap/btstack_gap_adv.c',
    'btstack/gap/btstack_gap_link.c',
    'btstack/gap/btstack_gap_privacy.c',
    'btstack/gap/btstack_gap_scan.c',
    'btstack/gap/btstack_gap_security.c',
    'btstack/include/gap/btstack_gap_adv.h',
    'btstack/include/gap/btstack_gap_link.h',
    'btstack/include/gap/btstack_gap_privacy.h',
    'btstack/include/gap/btstack_gap_scan.h',
    'btstack/include/gap/btstack_gap_security.h'
)

$failures = @()
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relative))) {
        $failures += "missing $relative"
    }
}
foreach ($relative in $forbidden) {
    if (Test-Path -LiteralPath (Join-Path $root $relative)) {
        $failures += "obsolete GAP unit remains: $relative"
    }
}

$headerPath = Join-Path $root 'btstack/include/gap/btstack_gap.h'
$gapPath = Join-Path $root 'btstack/gap/btstack_gap.c'
$allGapSources = ''
if (Test-Path -LiteralPath (Join-Path $root 'btstack/gap')) {
    $allGapSources = @(
        Get-ChildItem -LiteralPath (Join-Path $root 'btstack/gap') `
            -Filter '*.c' -File |
            ForEach-Object {
                Get-Content -Raw -LiteralPath $_.FullName
            }
    ) -join "`n"
}

if (Test-Path -LiteralPath $headerPath) {
    $header = Get-Content -Raw -LiteralPath $headerPath
    $publicApis = @(
        'btstack_gap_init',
        'btstack_gap_reset',
        'btstack_gap_set_event_callback',
        'btstack_gap_set_power',
        'btstack_gap_is_working',
        'btstack_gap_get_local_address'
    )
    $eventNames = @(
        'BTSTACK_GAP_EVENT_STACK_STATE',
        'BTSTACK_GAP_EVENT_LOCAL_ADDRESS',
        'BTSTACK_GAP_EVENT_ACL_CONNECTED',
        'BTSTACK_GAP_EVENT_ACL_DISCONNECTED',
        'BTSTACK_GAP_EVENT_INQUIRY_RESULT',
        'BTSTACK_GAP_EVENT_INQUIRY_COMPLETE',
        'BTSTACK_GAP_EVENT_PAIRING_REQUEST',
        'BTSTACK_GAP_EVENT_PAIRING_COMPLETE'
    )
    foreach ($symbol in $publicApis + $eventNames) {
        if ($header -notmatch [regex]::Escape($symbol)) {
            $failures += "public GAP contract missing $symbol"
        }
    }
}

$operationApis = @(
    'btstack_gap_set_local_name',
    'btstack_gap_set_class_of_device',
    'btstack_gap_set_discoverable',
    'btstack_gap_set_connectable',
    'btstack_gap_disconnect',
        'btstack_gap_classic_scan_start',
        'btstack_gap_classic_scan_stop',
        'btstack_gap_classic_is_scanning',
    'btstack_gap_set_pin',
    'btstack_gap_set_ssp_mode',
    'btstack_gap_pairing_confirm'
)
foreach ($symbol in $operationApis) {
    if ($allGapSources -notmatch [regex]::Escape($symbol)) {
        $failures += "GAP operation missing $symbol"
    }
}

$registrationCount = [regex]::Matches(
    $allGapSources,
    '\bhci_add_event_handler\s*\(').Count
if ($registrationCount -ne 1) {
    $failures += "expected one GAP HCI registration, found $registrationCount"
}

if (Test-Path -LiteralPath $gapPath) {
    $gap = Get-Content -Raw -LiteralPath $gapPath
    $sourceEvents = @(
        'BTSTACK_EVENT_STATE',
        'HCI_EVENT_CONNECTION_COMPLETE',
        'HCI_EVENT_DISCONNECTION_COMPLETE',
        'HCI_EVENT_PIN_CODE_REQUEST',
        'HCI_EVENT_USER_CONFIRMATION_REQUEST',
        'HCI_EVENT_SIMPLE_PAIRING_COMPLETE',
        'GAP_EVENT_INQUIRY_RESULT',
        'GAP_EVENT_INQUIRY_COMPLETE'
    )
    foreach ($event in $sourceEvents) {
        if ($allGapSources -notmatch [regex]::Escape($event)) {
            $failures += "GAP event mapping missing $event"
        }
    }
    if (($allGapSources -notmatch 'hci_event_connection_complete_get_link_type') -or
        ($allGapSources -notmatch 'HCI_LINK_TYPE_ACL')) {
        $failures += 'GAP ACL mapping does not reject SCO/eSCO links'
    }
}

$backendPath = Join-Path $root 'btstack/at/btstack_at_backend.c'
if (Test-Path -LiteralPath $backendPath) {
    $backend = Get-Content -Raw -LiteralPath $backendPath
    $forbiddenBackendPatterns = @(
        '\bhci_add_event_handler\s*\(',
        '\bhci_event_[A-Za-z0-9_]+\s*\(',
        '\bgap_event_[A-Za-z0-9_]+\s*\(',
        '(?<!btstack_)\bgap_[A-Za-z0-9_]+\s*\(',
        '\bhci_power_control\s*\('
    )
    foreach ($pattern in $forbiddenBackendPatterns) {
        if ($backend -match $pattern) {
            $failures += "AT backend bypasses SDK GAP layer: $pattern"
        }
    }
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Host "FAIL: $_" }
    exit 1
}

Write-Host 'BTstack independent GAP layer contract is clean'
