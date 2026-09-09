$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$canonicalRelative = 'btstack/include/btstack_config.h'
$canonical = Join-Path $root $canonicalRelative
$portRelative = 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$port = Join-Path $root $portRelative

$configs = @(Get-ChildItem -LiteralPath (Join-Path $root 'btstack') `
    -Recurse -File -Filter 'btstack_config.h' | ForEach-Object {
        $_.FullName.Substring($root.Length + 1).Replace('\', '/')
    })
if (($configs.Count -ne 1) -or ($configs[0] -ne $canonicalRelative)) {
    throw "Expected exactly $canonicalRelative; found: $($configs -join ', ')"
}
if (-not (Test-Path -LiteralPath $port -PathType Leaf)) {
    throw "Missing renamed GPCV2178 configuration: $portRelative"
}

$configText = Get-Content -Raw -LiteralPath $canonical
$requiredMacros = @(
    'BTSTACK_CORE',
    'BTSTACK_CLASSIC',
    'BTSTACK_LE',
    'BTSTACK_A2DP_SINK',
    'BTSTACK_HFP_HF',
    'BTSTACK_AVRCP_CONTROLLER',
    'BTSTACK_AVRCP_TARGET',
    'BTSTACK_PBAP_CLIENT',
    'BTSTACK_HID_DEVICE',
    'BTSTACK_GATT_CLIENT',
    'BTSTACK_GATT_SERVER',
    'BTSTACK_HOGP_DEVICE',
    'BTSTACK_BONDING'
)
foreach ($macro in $requiredMacros) {
    if ($configText -notmatch "(?m)^#define\s+$macro\s+[01]\s*$") {
        throw "Canonical config must define $macro as 0 or 1"
    }
}

$portText = Get-Content -Raw -LiteralPath $port
if ($portText -notmatch '#include\s+"btstack_config\.h"') {
    throw 'GPCV2178 config must include the canonical btstack_config.h'
}
foreach ($macro in $requiredMacros) {
    if ($portText -match "(?m)^#define\s+$macro\b") {
        throw "GPCV2178 config must not redefine protocol feature $macro"
    }
}

$legacyMacros = @(
    'BTSTACK_ENABLE_CORE',
    'BTSTACK_ENABLE_CLASSIC',
    'BTSTACK_ENABLE_BLE',
    'BTSTACK_ENABLE_A2DP_SINK',
    'BTSTACK_ENABLE_HFP_HF',
    'BTSTACK_ENABLE_PBAP_CLIENT',
    'BTSTACK_ENABLE_HID_DEVICE',
    'BTSTACK_ENABLE_HOGP_DEVICE',
    'BTSTACK_ENABLE_GATT',
    'BTSTACK_ENABLE_BONDING'
)
$productionFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'btstack') `
    -Recurse -File | Where-Object {
        ($_.Extension -in @('.c', '.h')) -and
        ($_.FullName -notmatch '[\\/]tests[\\/]')
    })
foreach ($file in $productionFiles) {
    $text = Get-Content -Raw -LiteralPath $file.FullName
    foreach ($macro in $legacyMacros) {
        if ($text -match "\b$macro\b") {
            throw "Legacy feature macro $macro remains in $($file.FullName)"
        }
    }
}

$typeHeader = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/include/adapter/btstack_types.h')
if ($typeHeader -notmatch '#include\s+"btstack_config\.h"') {
    throw 'Public BTstack types must include the canonical btstack_config.h'
}
if ($typeHeader -match 'adapter/btstack_config\.h') {
    throw 'Public BTstack types must not include the removed adapter config'
}

$profileGuards = @{
    'btstack/profiles/btstack_a2dp_sink.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_A2DP_SINK'
    'btstack/profiles/btstack_a2dp_media.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_A2DP_SINK'
    'btstack/profiles/btstack_hfp_hf.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_HFP_HF'
    'btstack/profiles/btstack_avrcp.c' =
        'BTSTACK_CLASSIC[\s\\]*&&[\s\\]*\(BTSTACK_AVRCP_CONTROLLER\s*\|\|\s*BTSTACK_AVRCP_TARGET\)'
    'btstack/profiles/btstack_avrcp_controller.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_AVRCP_CONTROLLER'
    'btstack/profiles/btstack_avrcp_target.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_AVRCP_TARGET'
    'btstack/profiles/btstack_pbap_client.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_PBAP_CLIENT'
    'btstack/profiles/btstack_hid_device.c' =
        'BTSTACK_CLASSIC\s*&&\s*BTSTACK_HID_DEVICE'
    'btstack/profiles/btstack_hogp_device.c' =
        'BTSTACK_LE\s*&&\s*BTSTACK_HOGP_DEVICE'
    'btstack/profiles/btstack_gatt_server.c' =
        'BTSTACK_LE\s*&&\s*BTSTACK_GATT_SERVER'
    'btstack/profiles/btstack_gatt_client.c' =
        'BTSTACK_LE\s*&&\s*BTSTACK_GATT_CLIENT'
}
foreach ($entry in $profileGuards.GetEnumerator()) {
    $source = Get-Content -Raw -LiteralPath (Join-Path $root $entry.Key)
    if ($source -notmatch $entry.Value) {
        throw "$($entry.Key) must be owned by its family and profile switches"
    }
}

$managerSource = Get-Content -Raw -LiteralPath (
    Join-Path $root 'btstack/core/btstack_profile_manager.c')
if ($managerSource -notmatch '#if\s+BTSTACK_CLASSIC' -or
    $managerSource -match '#ifn?def\s+ENABLE_CLASSIC') {
    throw 'Profile Manager must use the canonical Classic family switch'
}
foreach ($macro in @('BTSTACK_A2DP_SINK', 'BTSTACK_HFP_HF',
        'BTSTACK_AVRCP_CONTROLLER', 'BTSTACK_AVRCP_TARGET',
        'BTSTACK_PBAP_CLIENT')) {
    if ($managerSource -notmatch "\b$macro\b") {
        throw "Profile Manager must reject unavailable profile $macro"
    }
}

$forbiddenLeGranular = @(
    'BTSTACK_LE_ADVERTISING', 'BTSTACK_LE_SCANNING',
    'BTSTACK_LE_CENTRAL', 'BTSTACK_LE_PERIPHERAL', 'BTSTACK_LE_PRIVACY'
)
foreach ($macro in $forbiddenLeGranular) {
    if ($configText -match "\b$macro\b") {
        throw "Generic LE behavior must be controlled by BTSTACK_LE, not $macro"
    }
}

Write-Host 'PASS: one canonical BTstack feature configuration'
