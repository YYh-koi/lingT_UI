$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$makeIncPath = Join-Path $root 'btstack/Makefile.inc'
$libraryProjectPath = Join-Path $root 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'
$generatedMakePath = Join-Path $root `
    'Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library_Debug.mk'
$mainProjectPath = Join-Path $root 'Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj'
$wifiDemoPath = Join-Path $root 'Project/wifi_demo/atbm_wifi_probe_demo.c'
$deliveryPatchPath = Join-Path $root 'patches/wireless_static_library_integration.patch'

$makeInc = Get-Content -Raw -LiteralPath $makeIncPath
$libraryProject = [xml](Get-Content -Raw -LiteralPath $libraryProjectPath)
$generatedMake = Get-Content -Raw -LiteralPath $generatedMakePath
$normalizedGeneratedMake = $generatedMake.Replace('\\', '/')
$mainProject = [xml](Get-Content -Raw -LiteralPath $mainProjectPath)
$wifiDemo = Get-Content -Raw -LiteralPath $wifiDemoPath
$deliveryPatch = (Get-Content -Raw -LiteralPath $deliveryPatchPath).Replace('\', '/')

$newSources = @(
    'core/btstack_adapter.c',
    'core/btstack_port_ops.c',
    'core/btstack_property.c',
    'core/btstack_event_queue.c',
    'at/btstack_at_command_table.c',
    'at/btstack_at_dispatch_table.c'
)
foreach ($source in $newSources) {
    $makeCount = [regex]::Matches(
        $makeInc.Replace('\', '/'),
        [regex]::Escape($source)).Count
    if ($makeCount -ne 1) {
        throw "Makefile source count for $source is $makeCount, expected 1"
    }

    $units = @($libraryProject.SelectNodes(
        '//Unit[contains(translate(@filename,"\","/"),"' + $source + '")]'))
    if ($units.Count -ne 1) {
        throw "BTstack project unit count for $source is $($units.Count), expected 1"
    }

    if ($normalizedGeneratedMake -notmatch [regex]::Escape($source)) {
        throw "Generated xStack makefile omits $source"
    }
}

$ringMakeCount = [regex]::Matches(
    $makeInc.Replace('\', '/'),
    [regex]::Escape('core/btstack_ring.c')).Count
if ($ringMakeCount -ne 1) {
    throw "Makefile source count for core/btstack_ring.c is $ringMakeCount, expected 1"
}

$xstackOutputs = @($libraryProject.SelectNodes('//Target/Option[@output]') |
    Where-Object { $_.output.Replace('\', '/') -match '/atbm6165/bt/xstack$' })
if ($xstackOutputs.Count -ne 1) {
    throw "xStack library output count=$($xstackOutputs.Count), expected 1"
}

$legacyOutputs = @($libraryProject.SelectNodes('//Target/Option[@output]') |
    Where-Object { $_.output.Replace('\', '/') -match '/wlan_lib/(btstack|xstack)$' })
if ($legacyOutputs.Count -ne 0) {
    throw "Legacy BTstack library output count=$($legacyOutputs.Count), expected 0"
}

$activeLibraries = @($mainProject.SelectNodes(
    '//Linker/Add[contains(@library,"libxstack_api.a")]') |
    Where-Object { $_.library.Replace('\', '/') -match '/atbm6165/bt_api/libxstack_api\.a$' })
if ($activeLibraries.Count -ne 2) {
    throw "Active libxstack_api.a count=$($activeLibraries.Count), expected 2 (Debug and Release)"
}

$legacyLibraries = @($mainProject.SelectNodes(
    '//Linker/Add[contains(@library,"libbtstack.a") or contains(@library,"wlan_lib\libxstack.a") or contains(@library,"atbm6165\bt\libxstack.a")]'))
if ($legacyLibraries.Count -ne 0) {
    throw "Active libbtstack.a count=$($legacyLibraries.Count), expected 0"
}

if ($deliveryPatch -notmatch '(?m)^\+\s*<Add directory="\.\./\.\./gplib/liba_wifi_src/atbm/atbm6165/wlan_lib" />$') {
    throw 'Delivery patch does not add the WiFi public-header include directory'
}

if ($deliveryPatch -notmatch '(?m)^\+\s*<Add directory="\.\./\.\./gplib/liba_wifi_src/atbm/atbm6165/bt" />$') {
    throw 'Delivery patch does not add the Bluetooth public-header include directory'
}

if ($wifiDemo -match 'btstack_gpcv2178_start|ATBM_WIFI_BTSTACK_HOST_START|atbm_wifi_platform_btstack_start') {
    throw 'WiFi demo still owns BTstack startup'
}

Write-Host 'libxstack core integration is present once and decoupled from WiFi startup'
