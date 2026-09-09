param(
    [string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)

$ErrorActionPreference = 'Stop'
$failures = New-Object System.Collections.Generic.List[string]

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        $script:failures.Add($Message)
    }
}

function Get-WorkspacePath {
    param([string]$RelativePath)
    return Join-Path $WorkspaceRoot ($RelativePath -replace '/', '\')
}

$privateFiles = @(
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/core/atbm_gp_sdio_irq.h',
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_card.h',
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_l2.h',
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c',
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_cis.c',
    'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_l2.c'
)

foreach ($relativePath in $privateFiles) {
    Assert-True -Condition (Test-Path (Get-WorkspacePath $relativePath)) `
        -Message "Missing ATBM-private SDIO file: $relativePath"
}

$projectPath = Get-WorkspacePath 'Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj'
$projectText = Get-Content -Raw $projectPath
$privateSourceUnits = @(
    '../../../gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c',
    '../../../gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_cis.c',
    '../../../gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_l2.c'
)

foreach ($sourceUnit in $privateSourceUnits) {
    Assert-True -Condition $projectText.Contains($sourceUnit) `
        -Message "ATBM library project does not compile: $sourceUnit"
}

Assert-True -Condition (-not $projectText.Contains('../../../gplib/liba_wifi_src/rtk/8189FTV/platform/sdio')) `
    -Message 'ATBM library project still includes RTK/8189FTV SDIO directories'

$osSdioHeaderPath = Get-WorkspacePath 'gplib/liba_wifi_src/atbm/atbm6165/os/freertos/include/atbm_os_sdio.h'
$osSdioHeaderText = Get-Content -Raw $osSdioHeaderPath
Assert-True -Condition $osSdioHeaderText.Contains('#include "atbm_gp_sdio_card.h"') `
    -Message 'ATBM OS SDIO header does not select the ATBM-private card types'
Assert-True -Condition (-not $osSdioHeaderText.Contains('#include "card.h"')) `
    -Message 'ATBM OS SDIO header can still resolve the RTK card header'

$interfacePath = Get-WorkspacePath 'gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c'
$interfaceText = Get-Content -Raw $interfacePath
Assert-True -Condition (-not $interfaceText.Contains('#include "atbm_gp_sdio_l2.h"')) `
    -Message 'High-level WiFi interface imports private SDIO card type definitions'
Assert-True -Condition $interfaceText.Contains('extern INT32S atbm_gp_sdio_init(INT32U device_id);') `
    -Message 'High-level WiFi interface is missing the narrow private SDIO init declaration'

$manifestPath = Get-WorkspacePath 'tools/wireless_lib/wireless_sources.json'
$manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
Assert-True -Condition ([int]$manifest.expected_counts.wifi -eq 96) `
    -Message "Manifest WiFi expected count is $($manifest.expected_counts.wifi), expected 96"
Assert-True -Condition (@($manifest.wifi).Count -eq 96) `
    -Message "Manifest WiFi source count is $(@($manifest.wifi).Count), expected 96"

foreach ($sourceUnit in $privateSourceUnits) {
    $manifestUnit = $sourceUnit.Substring(9)
    Assert-True -Condition (@($manifest.wifi) -contains $manifestUnit) `
        -Message "Manifest does not list: $manifestUnit"
}

$baselineHashes = [ordered]@{
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/core/sdio_irq.h' = '94DC98EB2C9B0B63AF7E98EAE10E46B5C8DD4003C0101A1209F1448AEF0768F7'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/include/card.h' = '5512FA4E2FACC87C168B09155D7A620D5F993CFEAD270B51687E81ECAC680FE4'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/include/drv_l2_sdio_rtk.h' = 'F54A6FB3D454850306C315DC6E833A0B857132EE9CD50EAEF6E42585366F08E2'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/source/card.c' = '07F74822FE7A5E4FFA9BD13677E20C5160104C74A98991107DF1F55FCD5ACA38'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/source/drv_l2_sdio_cis_rtk.c' = 'F1EA49F8406C24DCBC480142B662BD1A31534D936A7D1BF66A27805B72113D0C'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/source/drv_l2_sdio_rtk.c' = 'F696F8B5E8BD7DE5DB0265B0BDA2E49001F37A503015AB200A9D43B5F1CC9353'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/wifi_io.c' = 'F168D2A13BC7E4B60F253F647E8C5DE9D1E7BCE6B148AEAC200D94B86875F572'
    'gplib/liba_wifi_src/rtk/8189FTV/platform/sdio/wifi_io.h' = 'C7D4DBA90597F66DA4870189D9F5BDAC6703F8C1832031DF34E0C6A219B3501C'
}

foreach ($entry in $baselineHashes.GetEnumerator()) {
    $path = Get-WorkspacePath $entry.Key
    Assert-True -Condition (Test-Path $path) -Message "Missing original RTK file: $($entry.Key)"
    if (Test-Path $path) {
        $actualHash = (Get-FileHash -Algorithm SHA256 $path).Hash
        Assert-True -Condition ($actualHash -eq $entry.Value) `
            -Message "Original RTK file changed: $($entry.Key)"
    }
}

if ($privateFiles | ForEach-Object { Test-Path (Get-WorkspacePath $_) } | Where-Object { -not $_ }) {
    # Content assertions are deferred until every private file exists.
}
else {
    $privateText = ($privateFiles | ForEach-Object { Get-Content -Raw (Get-WorkspacePath $_) }) -join "`n"
    Assert-True -Condition (-not $privateText.Contains('8189FTV')) `
        -Message 'ATBM-private SDIO files contain an 8189FTV dependency'
    Assert-True -Condition (-not $privateText.Contains('drv_l2_sdc.h')) `
        -Message 'ATBM-private SDIO files include drv_l2_sdc.h'
    Assert-True -Condition (-not $privateText.Contains('drvl2_sdc_clock_gate_enable')) `
        -Message 'ATBM-private SDIO files call drvl2_sdc_clock_gate_enable'
    Assert-True -Condition (-not ($privateText -match '\b(u8|u16|u32)\b')) `
        -Message 'ATBM-private SDIO files retain RTK scalar type aliases'

    foreach ($requiredSymbol in @(
        'atbm_gp_sdio_init',
        'atbm_gp_sdio_rw_direct',
        'atbm_gp_sdio_claim_irq',
        'atbm_gp_sdio0_irq_handler',
        'atbm_gp_sdio1_irq_handler'
    )) {
        Assert-True -Condition $privateText.Contains($requiredSymbol) `
            -Message "ATBM-private SDIO implementation is missing symbol: $requiredSymbol"
    }

    $privateCardHeader = Get-Content -Raw (
        Get-WorkspacePath 'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_card.h')
    foreach ($privateType in @(
        '#define sdio_func atbm_gp_sdio_func',
        '#define mmc_card atbm_gp_mmc_card',
        '#define mmc_host atbm_gp_mmc_host'
    )) {
        Assert-True -Condition $privateCardHeader.Contains($privateType) `
            -Message "ATBM-private SDIO card types are not isolated: $privateType"
    }
}

if ($failures.Count -gt 0) {
    Write-Host "ATBM private SDIO L2 contract failed ($($failures.Count)):" -ForegroundColor Red
    foreach ($failure in $failures) {
        Write-Host " - $failure" -ForegroundColor Red
    }
    exit 1
}

Write-Host 'ATBM private SDIO L2 contract passed.' -ForegroundColor Green
