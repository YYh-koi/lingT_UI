$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$profilePath = Join-Path $root 'btstack/profiles/btstack_avrcp.c'

if (-not (Test-Path -LiteralPath $profilePath)) {
    throw "Missing AVRCP profile source: $profilePath"
}

$profile = Get-Content -Raw -LiteralPath $profilePath

foreach ($removedToken in @(
        'BTSTACK_AVRCP_METADATA_WATCHDOG_MS',
        'btstack_avrcp_metadata_watchdog_timer',
        'btstack_avrcp_metadata_watchdog_arm',
        'btstack_avrcp_metadata_watchdog_cancel',
        'btstack_avrcp_metadata_watchdog_timeout',
        'metadata watchdog timeout')) {
    if ($profile -match [regex]::Escape($removedToken)) {
        throw "Obsolete wrapper watchdog remains: $removedToken"
    }
}

foreach ($requiredToken in @(
        'btstack_avrcp_metadata_inflight',
        'btstack_avrcp_metadata_complete',
        'BTSTACK_AVRCP_PENDING_METADATA')) {
    if ($profile -notmatch [regex]::Escape($requiredToken)) {
        throw "Required AVRCP request serialization was removed: $requiredToken"
    }
}

Write-Host 'AVRCP Metadata keeps request serialization without wrapper watchdog'
