$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourceNames = @(
    'btstack_adapter.c',
    'btstack_port_ops.c',
    'btstack_property.c'
)
$newCommonSourceNames = @(
    'btstack_platform.c',
    'btstack_ring.c',
    'btstack_call_audio_tx.c'
)
$runtimeCommonSourceNames = @(
    'btstack_core_dispatcher.c',
    'btstack_event_mapper.c',
    'btstack_runtime.c'
)
$gpcv2178PortSourceNames = @(
    'btstack_gpcv2178_port.c',
    'btstack_platform_gpcv2178.c',
    'btstack_rtos_gpcv2178.c',
    'btstack_hci_dump_gpcv2178.c',
    'btstack_run_loop_gpcv2178.c',
    'btstack_uart_gpcv2178.c',
    'btstack_control_gpcv2178.c',
    'btstack_storage_gpcv2178.c'
)

$adapterDirectory = Join-Path $repoRoot 'btstack\adapter'
if (Test-Path -LiteralPath $adapterDirectory) {
    throw 'Obsolete btstack/adapter directory still exists'
}

foreach ($requiredPath in @(
        'btstack\Makefile.inc',
        'btstack\README.md',
        'btstack\core\tests\test_btstack_default_name.ps1',
        'btstack\core\tests\test_bt_xstack_build_integration.ps1')) {
    if (-not (Test-Path -LiteralPath (Join-Path $repoRoot $requiredPath) -PathType Leaf)) {
        throw "Migrated core artifact is missing: $requiredPath"
    }
}

function Assert-LiteralCount(
    [string]$text,
    [string]$literal,
    [int]$expected,
    [string]$message) {
    $actual = ([regex]::Matches($text, [regex]::Escape($literal))).Count
    if ($actual -ne $expected) {
        throw "$message (expected=$expected actual=$actual)"
    }
}

function Get-ActiveMakefileText([string]$makefileText) {
    return (@($makefileText -split "`r?`n" | ForEach-Object { $_ -replace '\s*#.*$', '' }) -join "`n")
}

function Get-BTstackAdapterSourcesText([string]$makefileText) {
    $lines = @((Get-ActiveMakefileText $makefileText) -split "`r?`n")
    $result = @()
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ($lines[$index] -match '^\s*BTSTACK_ADAPTER_SOURCES\s*(?:\?|\+|:)?=') {
            do {
                $result += $lines[$index]
                $continues = $lines[$index] -match '\\\s*$'
                $index++
            } while ($continues -and $index -lt $lines.Count)
            $index--
        }
    }
    return ($result -join "`n")
}

function Assert-ActiveMakefileSelfTest {
    $commentOnly = Get-ActiveMakefileText '# $(BTSTACK_ROOT)/core/btstack_platform.c'
    Assert-LiteralCount $commentOnly 'core/btstack_platform.c' 0 `
        'Comment-only Make source must not satisfy source count'
    $active = Get-ActiveMakefileText @'
# $(BTSTACK_ROOT)/core/btstack_platform.c
BTSTACK_SOURCES := $(BTSTACK_ROOT)/core/btstack_platform.c
'@
    Assert-LiteralCount $active 'core/btstack_platform.c' 1 `
        'Active Make source must satisfy source count exactly once'
    $unused = Get-BTstackAdapterSourcesText @'
UNUSED_FIXTURE := $(BTSTACK_ROOT)/core/btstack_adapter.c
'@
    Assert-LiteralCount $unused 'core/btstack_adapter.c' 0 `
        'Unrelated Make variable must not satisfy BTSTACK_ADAPTER_SOURCES count'
    $adapterSources = Get-BTstackAdapterSourcesText @'
BTSTACK_ADAPTER_SOURCES := \
    $(BTSTACK_ROOT)/core/btstack_adapter.c
'@
    Assert-LiteralCount $adapterSources 'core/btstack_adapter.c' 1 `
        'BTSTACK_ADAPTER_SOURCES must satisfy source count exactly once'
}

function Get-ProjectUnitPaths([string]$projectText, [string]$projectPath) {
    [xml]$project = $projectText
    return @($project.SelectNodes('//Unit') |
        ForEach-Object {
            [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $projectPath) ([string]$_.filename)))
        })
}

function Assert-ProjectUnitPathNormalizationSelfTest {
    $fixtureProject = @'
<Project>
    <Unit filename="..\..\..\btstack\core\btstack_platform.c" />
    <Unit filename="..\..\..\..\btstack\core\btstack_platform.c" />
</Project>
'@
    $fixturePath = Join-Path $repoRoot 'Project\WirelessLibraries\Fixture\fixture.gpcmprj'
    $unitPaths = @(Get-ProjectUnitPaths $fixtureProject $fixturePath)
    $platformPath = [IO.Path]::GetFullPath((Join-Path $repoRoot 'btstack\core\btstack_platform.c'))
    if (@($unitPaths | Where-Object { $_ -eq $platformPath }).Count -ne 1) {
        throw 'Project path normalization self-test did not recognize a Windows backslash source path exactly once'
    }
}

Assert-ProjectUnitPathNormalizationSelfTest
Assert-ActiveMakefileSelfTest

foreach ($sourceName in $sourceNames) {
    $corePath = Join-Path $repoRoot "btstack\core\$sourceName"
    $adapterPath = Join-Path $repoRoot "btstack\adapter\$sourceName"
    if (-not (Test-Path -LiteralPath $corePath -PathType Leaf)) {
        throw "Core source is missing: btstack/core/$sourceName"
    }
    if (Test-Path -LiteralPath $adapterPath) {
        throw "Old adapter source still exists: btstack/adapter/$sourceName"
    }
}

$obsoleteInternalHeader = Join-Path $repoRoot `
    'btstack\adapter\bt_xstack_manager_internal.h'
if (Test-Path -LiteralPath $obsoleteInternalHeader) {
    throw 'Obsolete bt_xstack_manager_internal.h still exists'
}

$makefilePath = Join-Path $repoRoot 'btstack\Makefile.inc'
$rawMakefile = Get-Content -Raw -LiteralPath $makefilePath
$fullMakefile = (Get-ActiveMakefileText $rawMakefile).Replace('\', '/')
$makefile = (Get-BTstackAdapterSourcesText $rawMakefile).Replace('\', '/')
foreach ($sourceName in $sourceNames) {
    Assert-LiteralCount $makefile "core/$sourceName" 1 `
        "Source manifest must contain core/$sourceName exactly once"
    Assert-LiteralCount $makefile "adapter/$sourceName" 0 `
        "Source manifest still contains adapter/$sourceName"
}
Assert-LiteralCount $makefile 'adapter/bt_xstack_manager.c' 0 `
    'Source manifest still contains the absent bt_xstack_manager.c'
foreach ($sourceName in $newCommonSourceNames) {
    Assert-LiteralCount $makefile "core/$sourceName" 1 `
        "Common source manifest must contain core/$sourceName exactly once"
}
Assert-LiteralCount $fullMakefile 'ports/gpcv2178' 0 `
    'Common Makefile must not select a concrete GPCV2178 port'

$portManifestPath = Join-Path $repoRoot 'btstack\ports\gpcv2178\Makefile.inc'
if (-not (Test-Path -LiteralPath $portManifestPath -PathType Leaf)) {
    throw 'GPCV2178 selected-port Makefile is missing'
}
$portManifest = (Get-ActiveMakefileText (Get-Content -Raw -LiteralPath $portManifestPath)).Replace('\', '/')
Assert-LiteralCount $portManifest 'ports/gpcv2178' 9 `
    'GPCV2178 port manifest must select one include directory and eight sources'
foreach ($sourceName in $gpcv2178PortSourceNames) {
    Assert-LiteralCount $portManifest "ports/gpcv2178/$sourceName" 1 `
        "GPCV2178 port manifest must contain $sourceName exactly once"
}

$projectPaths = @(
    'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj',
    'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library.gpcmprj',
    'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'
)
foreach ($relativeProjectPath in $projectPaths) {
    $projectPath = Join-Path $repoRoot $relativeProjectPath
    $unitPaths = @(Get-ProjectUnitPaths (Get-Content -Raw -LiteralPath $projectPath) $projectPath)
    foreach ($sourceName in $sourceNames) {
        $coreCount = @($unitPaths | Where-Object { $_ -eq (Join-Path $repoRoot "btstack\core\$sourceName") }).Count
        $adapterCount = @($unitPaths | Where-Object { $_ -eq (Join-Path $repoRoot "btstack\adapter\$sourceName") }).Count
        if ($coreCount -ne 1) {
            throw "$relativeProjectPath core/$sourceName count=$coreCount, expected 1"
        }
        if ($adapterCount -ne 0) {
            throw "$relativeProjectPath adapter/$sourceName count=$adapterCount, expected 0"
        }
    }
    foreach ($sourceName in $newCommonSourceNames) {
        $sourceCount = @($unitPaths | Where-Object {
                $_ -eq (Join-Path $repoRoot "btstack\core\$sourceName")
            }).Count
        if ($sourceCount -ne 1) {
            throw "$relativeProjectPath core/$sourceName count=$sourceCount, expected 1"
        }
    }
    foreach ($sourceName in $runtimeCommonSourceNames) {
        $sourceCount = @($unitPaths | Where-Object {
                $_ -eq (Join-Path $repoRoot "btstack\core\$sourceName")
            }).Count
        if ($sourceCount -ne 1) {
            throw "$relativeProjectPath core/$sourceName count=$sourceCount, expected 1"
        }
    }
    foreach ($sourceName in $gpcv2178PortSourceNames) {
        $sourceCount = @($unitPaths | Where-Object {
                $_ -eq (Join-Path $repoRoot "btstack\ports\gpcv2178\$sourceName")
            }).Count
        if ($sourceCount -ne 1) {
            throw "$relativeProjectPath ports/gpcv2178/$sourceName count=$sourceCount, expected 1"
        }
    }
    if (@($unitPaths | Where-Object {
                $_ -eq (Join-Path $repoRoot 'btstack\adapter\bt_xstack_manager.c')
            }).Count -ne 0) {
        throw "$relativeProjectPath still contains absent bt_xstack_manager.c"
    }
}

$manifestPath = Join-Path $repoRoot 'tools\wireless_lib\wireless_sources.json'
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifest = $manifestText | ConvertFrom-Json
foreach ($sourceName in $sourceNames) {
    $coreEntry = "btstack/core/$sourceName"
    $adapterEntry = "btstack/adapter/$sourceName"
    if (@($manifest.bluetooth | Where-Object { $_ -eq $coreEntry }).Count -ne 1) {
        throw "Wireless manifest must contain $coreEntry exactly once"
    }
    if (@($manifest.bluetooth | Where-Object { $_ -eq $adapterEntry }).Count -ne 0) {
        throw "Wireless manifest still contains $adapterEntry"
    }
}
foreach ($sourceName in $newCommonSourceNames) {
    $entry = "btstack/core/$sourceName"
    if (@($manifest.bluetooth | Where-Object { $_ -eq $entry }).Count -ne 1) {
        throw "Wireless manifest must contain $entry exactly once"
    }
}
foreach ($sourceName in $runtimeCommonSourceNames) {
    $entry = "btstack/core/$sourceName"
    if (@($manifest.bluetooth | Where-Object { $_ -eq $entry }).Count -ne 1) {
        throw "Wireless manifest must contain $entry exactly once"
    }
}
foreach ($sourceName in $gpcv2178PortSourceNames) {
    $entry = "btstack/ports/gpcv2178/$sourceName"
    if (@($manifest.bluetooth | Where-Object { $_ -eq $entry }).Count -ne 1) {
        throw "Wireless manifest must contain $entry exactly once"
    }
}
if (@($manifest.bluetooth | Where-Object {
            $_ -eq 'btstack/adapter/bt_xstack_manager.c'
        }).Count -ne 0) {
    throw 'Wireless manifest still contains absent bt_xstack_manager.c'
}
if ([int]$manifest.expected_counts.bluetooth -ne @($manifest.bluetooth).Count) {
    throw 'Wireless manifest Bluetooth expected count is stale'
}

$requiredPlatformPaths = @(
    'btstack\include\port',
    'btstack\core\btstack_platform.c',
    'btstack\ports\gpcv2178\btstack_platform_gpcv2178.c',
    'btstack\ports\gpcv2178\btstack_rtos_gpcv2178.c'
)
foreach ($requiredPath in $requiredPlatformPaths) {
    $fullPath = Join-Path $repoRoot $requiredPath
    $pathType = if ($requiredPath -eq 'btstack\include\port') { 'Container' } else { 'Leaf' }
    if (-not (Test-Path -LiteralPath $fullPath -PathType $pathType)) {
        throw "Required platform boundary artifact is missing: $requiredPath"
    }
}

$platformCoreSource = 'btstack/core/btstack_platform.c'
Assert-LiteralCount $makefile 'core/btstack_platform.c' 1 `
    'Source manifest must contain core/btstack_platform.c exactly once'
if (@($manifest.bluetooth | Where-Object { $_ -eq $platformCoreSource }).Count -ne 1) {
    throw 'Wireless manifest must contain btstack/core/btstack_platform.c exactly once'
}

$platformPortSources = @(
    'btstack/ports/gpcv2178/btstack_platform_gpcv2178.c',
    'btstack/ports/gpcv2178/btstack_rtos_gpcv2178.c'
)
foreach ($platformPortSource in $platformPortSources) {
    if (@($manifest.bluetooth | Where-Object { $_ -eq $platformPortSource }).Count -ne 1) {
        throw "Wireless manifest must contain $platformPortSource exactly once"
    }
}
foreach ($relativeProjectPath in $projectPaths) {
    $projectPath = Join-Path $repoRoot $relativeProjectPath
    $unitPaths = @(Get-ProjectUnitPaths (Get-Content -Raw -LiteralPath $projectPath) $projectPath)
    foreach ($platformSource in @($platformCoreSource) + $platformPortSources) {
        $sourceCount = @($unitPaths | Where-Object {
                $_ -eq (Join-Path $repoRoot $platformSource.Replace('/', '\'))
            }).Count
        if ($sourceCount -ne 1) {
            throw "$relativeProjectPath must contain $platformSource exactly once (actual=$sourceCount)"
        }
    }
}

foreach ($relativeGeneratedMakefile in @(
        'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library_Debug.mk',
        'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library_Debug.mk')) {
    $generatedText = (Get-Content -Raw -LiteralPath (Join-Path $repoRoot $relativeGeneratedMakefile)).Replace('\\', '/')
    foreach ($sourceName in $newCommonSourceNames) {
        $rulePattern = '(?m)^\$\(OBJDIR_DEBUG\)/btstack/core/' +
            [regex]::Escape([IO.Path]::ChangeExtension($sourceName, '.o')) +
            ': .*btstack/core/' + [regex]::Escape($sourceName) + '$'
        if ([regex]::Matches($generatedText, $rulePattern).Count -ne 1) {
            throw "$relativeGeneratedMakefile must contain one object rule for core/$sourceName"
        }
    }
    foreach ($sourceName in $gpcv2178PortSourceNames) {
        $rulePattern = '(?m)^\$\(OBJDIR_DEBUG\)/btstack/ports/gpcv2178/' +
            [regex]::Escape([IO.Path]::ChangeExtension($sourceName, '.o')) +
            ': .*btstack/ports/gpcv2178/' + [regex]::Escape($sourceName) + '$'
        if ([regex]::Matches($generatedText, $rulePattern).Count -ne 1) {
            throw "$relativeGeneratedMakefile must contain one object rule for ports/gpcv2178/$sourceName"
        }
    }
}

foreach ($generatedProject in @(
        @('Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj',
          'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library_Debug.mk'),
        @('Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library.gpcmprj',
          'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library_Debug.mk'),
        @('Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj',
          'Project\WirelessLibraries\BTstack_Library\BTstack_Library_Debug.mk'),
        @('Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj',
          'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library_Debug.mk'))) {
    [xml]$projectXml = Get-Content -Raw -LiteralPath (Join-Path $repoRoot $generatedProject[0])
    $projectUnits = @($projectXml.SelectNodes('//Unit'))
    $generatedPath = Join-Path $repoRoot $generatedProject[1]
    if (-not (Test-Path -LiteralPath $generatedPath -PathType Leaf)) {
        throw "Generated Makefile is missing: $($generatedProject[1])"
    }
    $generatedLines = @((Get-Content -Raw -LiteralPath $generatedPath) -split "`r?`n")
    $objectRules = @($generatedLines | Where-Object {
            $_ -match '^\$\(OBJDIR_DEBUG\)\\.+\.o: .+\.c$'
        })
    if ($objectRules.Count -ne $projectUnits.Count) {
        throw "$($generatedProject[1]) object rule count=$($objectRules.Count), expected=$($projectUnits.Count)"
    }
    $objectTargets = @($objectRules | ForEach-Object { ($_ -split ': ', 2)[0] })
    $sourceInputs = @($objectRules | ForEach-Object { ($_ -split ': ', 2)[1] })
    if ((@($objectTargets | Sort-Object -Unique).Count -ne $objectTargets.Count) -or
        (@($sourceInputs | Sort-Object -Unique).Count -ne $sourceInputs.Count)) {
        throw "$($generatedProject[1]) contains duplicate object targets or source rules"
    }
}

# These projects consume a selected Bluetooth archive and must not compile a
# second descriptor locally. The WiFi archive has no Bluetooth dependency;
# the turnkey target links the API archive exactly once.
[xml]$wifiProject = Get-Content -Raw -LiteralPath (Join-Path $repoRoot `
    'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj')
if (@($wifiProject.SelectNodes('//Unit') | Where-Object {
            ([string]$_.filename).Replace('\', '/') -match 'btstack/(?:core|ports)/'
        }).Count -ne 0) {
    throw 'ATBM WiFi library must not compile BTstack common or port sources'
}
$turnkeyText = (Get-Content -Raw -LiteralPath (Join-Path $repoRoot `
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj')).Replace('\', '/')
Assert-LiteralCount $turnkeyText 'bt_api/libxstack_api.a' 2 `
    'Turnkey has two configurations and each must link the selected API archive once'
if ($turnkeyText -match 'btstack/ports/gpcv2178/.+\.c') {
    throw 'Turnkey must consume the selected API archive, not compile a second port'
}

$compatHeaders = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'btstack\include\adapter') -File)
if (($compatHeaders.Count -ne 4) -or
    ((@($compatHeaders.Name | Sort-Object) -join ',') -ne
    (@(@('btstack_adapter.h','btstack_port_ops.h','btstack_property.h','btstack_types.h') | Sort-Object) -join ','))) {
    throw 'include/adapter must retain exactly four application-neutral compatibility headers'
}

$adapterHeaderText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot `
    'btstack\include\adapter\btstack_adapter.h')
$adapterSourceText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot `
    'btstack\core\btstack_adapter.c')
foreach ($requiredAdapterFunction in @(
        [PSCustomObject]@{ Name = 'btstack_adapter_setup'; ReturnType = 'int' },
        [PSCustomObject]@{ Name = 'btstack_adapter_deinit'; ReturnType = 'void' })) {
    if ([regex]::Matches($adapterHeaderText,
            "\b$($requiredAdapterFunction.Name)\s*\(").Count -ne 1) {
        throw "Adapter lifecycle header must declare $($requiredAdapterFunction.Name) exactly once"
    }
    if ([regex]::Matches($adapterSourceText,
            "\b$($requiredAdapterFunction.ReturnType)\s+$($requiredAdapterFunction.Name)\s*\(").Count -ne 1) {
        throw "Adapter lifecycle source must define $($requiredAdapterFunction.Name) exactly once"
    }
}
foreach ($obsoleteAdapterFunction in @(
        'btstack_adapter_start',
        'btstack_adapter_enable_transport',
        'btstack_adapter_enable_classic',
        'btstack_adapter_enable_ble',
        'btstack_adapter_set_auto_reconnect',
        'btstack_adapter_get_auto_reconnect',
        'btstack_adapter_set_adv_field',
        'btstack_adapter_set_eir_field',
        'btstack_adapter_apply_names',
        'btstack_adapter_apply_classic_name',
        'btstack_adapter_apply_ble_name',
        'btstack_adapter_is_bonded',
        'btstack_adapter_check_bond',
        'btstack_adapter_get_current_bond',
        'btstack_adapter_clear_bond',
        'btstack_adapter_select_bond_id',
        'btstack_adapter_get_bond_id',
        'btstack_adapter_get_bond_by_id')) {
    if (($adapterHeaderText -match "\b$obsoleteAdapterFunction\b") -or
        ($adapterSourceText -match "\b$obsoleteAdapterFunction\b")) {
        throw "Obsolete adapter facade remains: $obsoleteAdapterFunction"
    }
}

$projectFiles = @(
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj',
    'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
) + $projectPaths
foreach ($relativeProjectPath in $projectFiles) {
    $projectText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot $relativeProjectPath)
    if ($projectText.Replace('\', '/') -match 'btstack/adapter') {
        throw "$relativeProjectPath still contains a btstack/adapter path"
    }
}

Write-Output 'PASS: common sources and metadata migrated; btstack/adapter is absent'
