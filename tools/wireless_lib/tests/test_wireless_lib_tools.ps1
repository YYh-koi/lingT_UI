$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$workspaceRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$script:Failures = 0

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

function Assert-Equal {
    param(
        $Expected,
        $Actual,
        [string]$Message
    )

    if ($Expected -ne $Actual) {
        $script:Failures++
        Write-Host "FAIL: $Message (expected=$Expected actual=$Actual)" -ForegroundColor Red
    }
}

function Assert-Contains {
    param(
        [string]$Expected,
        [object[]]$Actual,
        [string]$Message
    )

    Assert-True -Condition ($Actual -contains $Expected) -Message $Message
}

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolDirectory = Split-Path -Parent $testDirectory
$workspace = (Resolve-Path (Join-Path $toolDirectory '..\..')).Path
$modulePath = Join-Path $toolDirectory 'WirelessLibTools.psm1'
$manifestPath = Join-Path $toolDirectory 'wireless_sources.json'
$projectPath = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'

Import-Module $modulePath -Force

$manifest = Read-WirelessSourceManifest -Path $manifestPath -WorkspaceRoot $workspace
Assert-Equal -Expected 96 -Actual $manifest.Wifi.Count -Message 'WiFi source count'
Assert-Equal -Expected 108 -Actual $manifest.Bluetooth.Count -Message 'BTstack source count'
Assert-Contains `
    -Expected (Join-Path $workspace 'btstack\core\btstack_adapter.c') `
    -Actual $manifest.Bluetooth `
    -Message 'BTstack core adapter is part of the Bluetooth archive manifest'
$missingSources = @($manifest.Wifi + $manifest.Bluetooth | Where-Object { -not (Test-Path -LiteralPath $_) })
Assert-Equal -Expected 0 -Actual $missingSources.Count -Message 'all manifest sources exist'

$coverArtSourceHeader = Join-Path $workspace 'btstack\include\api\bt_xstack_cover_art.h'
$coverArtDeliveryHeader = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\bt_api\bt_xstack_cover_art.h'
Assert-True -Condition (Test-Path -LiteralPath $coverArtDeliveryHeader -PathType Leaf) `
    -Message 'xStack API delivery includes the cover-art public header'
if (Test-Path -LiteralPath $coverArtDeliveryHeader -PathType Leaf) {
    Assert-Equal `
        -Expected (Get-FileHash -LiteralPath $coverArtSourceHeader -Algorithm SHA256).Hash `
        -Actual (Get-FileHash -LiteralPath $coverArtDeliveryHeader -Algorithm SHA256).Hash `
        -Message 'xStack cover-art delivery header matches the API source header'
}
$btstackPortConfig = Get-Content -LiteralPath (Join-Path $workspace 'btstack\ports\gpcv2178\btstack_gpcv2178_config.h') -Raw
Assert-True -Condition ($btstackPortConfig -match '(?m)^#define\s+ENABLE_GOEP_L2CAP\s*$') `
    -Message 'cover-art transport enables GOEP over L2CAP'
Assert-True -Condition ($btstackPortConfig -match '(?m)^#define\s+ENABLE_L2CAP_ENHANCED_RETRANSMISSION_MODE\s*$') `
    -Message 'GOEP over L2CAP enables enhanced retransmission mode'

$config = Get-DebugCompilerConfiguration -ProjectFile $projectPath
Assert-Contains -Expected '-O2' -Actual $config.Options -Message 'optimization option'
Assert-Contains -Expected '-mcpu=cortex-a7' -Actual $config.Options -Message 'CPU ABI option'
Assert-Contains -Expected '-mfloat-abi=softfp' -Actual $config.Options -Message 'float ABI option'
Assert-Contains -Expected '-DWIFI_VENDOR=3' -Actual $config.Options -Message 'WiFi vendor option'

$debugInputs = Get-DebugProjectInputs -ProjectFile $projectPath -WorkspaceRoot $workspace
Assert-Equal -Expected 287 -Actual $debugInputs.CompileUnits.Count -Message 'active Debug compile root count'
Assert-Equal -Expected 19 -Actual $debugInputs.Archives.Count -Message 'explicit Debug archive count'
Assert-Equal -Expected 1 -Actual $debugInputs.LinkerScripts.Count -Message 'Debug linker script count'
Assert-Equal -Expected 0 -Actual @(
    $debugInputs.CompileUnits |
        Where-Object { $_ -match '[\\/](btstack|atbm6165)[\\/].+\.c$' }
).Count -Message 'commented wireless units are not active compile roots'
Assert-Equal -Expected 88 -Actual @(
    $debugInputs.CompileUnits |
        Where-Object { $_ -match '[\\/]liba_wifi_src[\\/]atbm[\\/]network[\\/].+\.c$' }
).Count -Message 'ATBM network sources remain main-project compile roots'

$dependencyBatches = New-GccDependencyBatches `
    -CompileUnits $debugInputs.CompileUnits `
    -BatchSize 40
Assert-Equal -Expected 9 -Actual $dependencyBatches.Count -Message 'dependency batch count'
Assert-Equal -Expected 287 -Actual (
    @($dependencyBatches | ForEach-Object { $_.Sources }).Count
) -Message 'all compile roots assigned to dependency batches'
Assert-Equal -Expected 7 -Actual @(
    $dependencyBatches | Where-Object { $_.Language -eq 'c' }
).Count -Message 'C dependency batch count'
Assert-Equal -Expected 1 -Actual @(
    $dependencyBatches | Where-Object { $_.Language -eq 'c++' }
).Count -Message 'C++ dependency batch count'
Assert-Equal -Expected 1 -Actual @(
    $dependencyBatches | Where-Object { $_.Language -eq 'assembler-with-cpp' }
).Count -Message 'assembly dependency batch count'

$dependencyFixture = Join-Path $workspace (
    'output\wireless_libs\test-dependency-fixture-' + [guid]::NewGuid().ToString('N')
)
New-Item -ItemType Directory -Path (Join-Path $dependencyFixture 'inc') -Force | Out-Null
$dependencySource = Join-Path $dependencyFixture 'active.c'
$dependencyHeader1 = Join-Path $dependencyFixture 'inc\used.h'
$dependencyHeader2 = Join-Path $dependencyFixture 'inc\nested.h'
Set-Content -LiteralPath $dependencySource -Value '#include "used.h"'
Set-Content -LiteralPath $dependencyHeader1 -Value '#include "nested.h"'
Set-Content -LiteralPath $dependencyHeader2 -Value '#define DEPENDENCY_FIXTURE 1'
$dependencyOutput = @(
    'dependency-target: ' + $dependencySource.Replace('\', '/') + ' \',
    ' ' + $dependencyHeader1.Replace('\', '/') + ' \',
    ' ' + $dependencyHeader2.Replace('\', '/') + ' C:/toolchain/include/stdint.h'
)
$dependencyPaths = ConvertFrom-GccDependencyOutput `
    -Output @($dependencyOutput + '') `
    -WorkspaceRoot $workspace
Assert-Equal -Expected 3 -Actual $dependencyPaths.Count -Message 'workspace dependency path count'
Assert-Contains -Expected $dependencySource -Actual $dependencyPaths -Message 'dependency source retained'
Assert-Contains -Expected $dependencyHeader2 -Actual $dependencyPaths -Message 'recursive dependency retained'
Assert-Equal -Expected 0 -Actual @(
    $dependencyPaths |
        Where-Object { -not $_.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase) }
).Count -Message 'toolchain dependencies excluded'

$mappedDependency = Convert-PathBetweenRoots `
    -Path $dependencyHeader1 `
    -SourceRoot $workspace `
    -DestinationRoot 'D:\candidate-sdk'
Assert-Equal `
    -Expected 'D:\candidate-sdk\output\wireless_libs\test-dependency-fixture-' `
    -Actual $mappedDependency.Substring(0, 'D:\candidate-sdk\output\wireless_libs\test-dependency-fixture-'.Length) `
    -Message 'workspace path maps into candidate root'
$outsidePathRejected = $false
try {
    Convert-PathBetweenRoots `
        -Path 'C:\outside\escape.h' `
        -SourceRoot $workspace `
        -DestinationRoot 'D:\candidate-sdk' | Out-Null
}
catch {
    $outsidePathRejected = $true
}
Assert-True -Condition $outsidePathRejected -Message 'path mapping rejects source outside root'

$toolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$planOutput = Join-Path $workspace 'output\wireless_libs\test-plan'
$compilePlan = New-WirelessCompilePlan `
    -Manifest $manifest `
    -Configuration $config `
    -ToolchainDirectory $toolchainDirectory `
    -OutputDirectory $planOutput `
    -WorkspaceRoot $workspace
Assert-Equal -Expected (Join-Path $toolchainDirectory 'arm-none-eabi-gcc.exe') -Actual $compilePlan.Compiler -Message 'Generalplus compiler path'
Assert-Equal -Expected 96 -Actual $compilePlan.WifiActions.Count -Message 'WiFi compile action count'
Assert-Equal -Expected $manifest.Bluetooth.Count -Actual $compilePlan.BluetoothActions.Count -Message 'BTstack compile action count'
$allObjectPaths = @($compilePlan.WifiActions.ObjectPath + $compilePlan.BluetoothActions.ObjectPath)
Assert-Equal -Expected ($manifest.Wifi.Count + $manifest.Bluetooth.Count) -Actual @($allObjectPaths | Sort-Object -Unique).Count -Message 'unique object paths'
Assert-Contains -Expected '-mcpu=cortex-a7' -Actual $compilePlan.Options -Message 'compile plan CPU ABI'
$expectedBtConfig = Join-Path $workspace 'btstack\ports\gpcv2178\btstack_gpcv2178_config.h'
Assert-Equal -Expected '-include' -Actual $compilePlan.BluetoothActions[0].AdditionalArguments[0] -Message 'BTstack config preinclude option'
Assert-Equal -Expected $expectedBtConfig -Actual $compilePlan.BluetoothActions[0].AdditionalArguments[1] -Message 'BTstack target config preinclude path'
$expectedSbcInclude = Join-Path $workspace 'btstack\3rd-party\bluedroid\decoder\include'
Assert-Contains -Expected $expectedSbcInclude -Actual $compilePlan.IncludeDirectories -Message 'BTstack SBC decoder include path'
$compilerVersion = & $compilePlan.Compiler --version | Select-Object -First 1
Assert-True -Condition ($compilerVersion -match '4\.9\.3') -Message 'Generalplus GCC 4.9.3 selected'

$diagnosticOutput = Invoke-CheckedProcess `
    -FilePath $env:COMSPEC `
    -Arguments @('/d', '/c', 'echo harmless diagnostic 1>&2 & exit /b 0') `
    -Description 'stderr success probe' `
    -Quiet
Assert-True -Condition ((@($diagnosticOutput) -join "`n") -match 'harmless diagnostic') -Message 'successful stderr is captured'

$archiveArguments = New-ArchiveArguments `
    -ArchivePath 'C:\out\libexample.a' `
    -ObjectPaths @('C:\obj\a.o', 'C:\obj\b.o')
Assert-Equal -Expected 4 -Actual $archiveArguments.Count -Message 'archive argument count'
Assert-Equal -Expected 'rcsD' -Actual $archiveArguments[0] -Message 'deterministic archive flags'
Assert-Equal -Expected 'C:\obj\b.o' -Actual $archiveArguments[3] -Message 'archive object ordering'

[xml]$integratedProject = Get-Content -LiteralPath $projectPath -Raw
$wirelessUnits = @($integratedProject.emIDE_project_file.Project.Unit |
    Where-Object {
        $_.HasAttribute('filename') -and
        ([string]$_.filename -match '(liba_wifi_src/atbm/atbm6165|btstack).+\.c$')
    })
Assert-Equal -Expected 0 -Actual $wirelessUnits.Count -Message 'project has no direct wireless C units'
$debugTarget = @($integratedProject.emIDE_project_file.Project.Build.Target |
    Where-Object { [string]$_.title -eq 'Debug' }) | Select-Object -First 1
$debugLibraries = @($debugTarget.Linker.Add |
    Where-Object { $_.HasAttribute('library') } |
    ForEach-Object { [string]$_.library })
$wirelessLibraryRoot = '..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib'
$bluetoothApiLibraryRoot = '..\..\gplib\liba_wifi_src\atbm\atbm6165\bt_api'
Assert-Equal -Expected 1 -Actual @(
    $debugLibraries |
        Where-Object { $_ -eq "$wirelessLibraryRoot\libatbm_wifi.a" }
).Count -Message 'WiFi archive linked once from ATBM wlan_lib'
Assert-Equal -Expected 1 -Actual @(
    $debugLibraries |
        Where-Object { $_ -eq "$bluetoothApiLibraryRoot\libxstack_api.a" }
).Count -Message 'xStack API archive linked once from ATBM bt_api directory'
Assert-Equal -Expected 0 -Actual @(
    $debugLibraries |
        Where-Object { $_ -eq "$wirelessLibraryRoot\libxstack.a" }
).Count -Message 'xStack archive is not linked from wlan_lib'

$stageCommand = Get-Command New-WirelessCustomerStage -ErrorAction SilentlyContinue
Assert-True -Condition ($null -ne $stageCommand) -Message 'customer staging function exists'
$packageScriptText = Get-Content -LiteralPath (Join-Path $toolDirectory 'package_wireless_release.ps1') -Raw
$buildScriptText = Get-Content -LiteralPath (Join-Path $toolDirectory 'build_wireless_libs.ps1') -Raw
Assert-True `
    -Condition (($buildScriptText -match 'CustomerWirelessRoot') -and
        ($buildScriptText -match 'atbm6165')) `
    -Message 'CLI library builder publishes from the ATBM wireless root'
Assert-True `
    -Condition ($packageScriptText -match '-CustomerStripTool\s+\$stripTool') `
    -Message 'release packager strips staged customer archives'
Assert-True `
    -Condition ($packageScriptText -match 'Get-DebugProjectInputs') `
    -Message 'release packager parses active Debug inputs'
Assert-True `
    -Condition ($packageScriptText -match 'Get-GccProjectDependencies') `
    -Message 'release packager derives GCC dependency closure'
Assert-True `
    -Condition ($packageScriptText -match 'New-MinimalWirelessCustomerStage') `
    -Message 'release packager uses minimal customer staging'
Assert-True `
    -Condition ($packageScriptText -notmatch '\bNew-WirelessCustomerStage\b') `
    -Message 'release packager no longer uses broad SDK copy staging'
$minimalVerifierPath = Join-Path $toolDirectory 'verify_minimal_customer_build.ps1'
Assert-True `
    -Condition (Test-Path -LiteralPath $minimalVerifierPath -PathType Leaf) `
    -Message 'minimal customer compile/link verifier exists'
if ($null -ne $stageCommand) {
    $fixtureRoot = Join-Path $workspace ('output\wireless_libs\test-stage-fixture-' + [guid]::NewGuid().ToString('N'))
    $stageRoot = Join-Path $fixtureRoot 'stage'
    foreach ($directory in @(
        'gplib\liba_wifi_src\atbm\atbm6165\bt',
        'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib',
        'patches',
        'docs',
        'Project\GPCV2178_Turnkey_SDK',
        'Project\WirelessLibraries\ATBM_WiFi_Library',
        'application',
        'gplib\liba_wifi_src\atbm\atbm6165\include',
        'btstack\src',
        'atbm_rtos_wifi6',
        'fake-tools'
    )) {
        New-Item -ItemType Directory -Path (Join-Path $fixtureRoot $directory) -Force | Out-Null
    }
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a') -Value 'wifi archive fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h') -Value '#define ATBM_WIFI_PUBLIC 1'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a') -Value 'bt archive fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h') -Value '#define BT_XSTACK_PUBLIC 1'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'patches\wireless_static_library_integration.patch') -Value 'patch fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'docs\wireless_static_library_integration.md') -Value 'guide fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj') -Value 'project fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj') -Value 'internal project fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'application\keep.c') -Value 'non-wireless source fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\include\wifi.h') -Value 'wifi header fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\wifi.c') -Value 'wifi source fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'btstack\src\btstack.h') -Value 'bt header fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'btstack\src\btstack.c') -Value 'bt source fixture'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'atbm_rtos_wifi6\duplicate.c') -Value 'duplicate source fixture'

    $stripLog = Join-Path $fixtureRoot 'strip.log'
    $ranlibLog = Join-Path $fixtureRoot 'ranlib.log'
    $fakeStrip = Join-Path $fixtureRoot 'fake-tools\strip.cmd'
    $fakeRanlib = Join-Path $fixtureRoot 'fake-tools\ranlib.cmd'
    Set-Content -LiteralPath $fakeStrip -Value @(
        '@echo off',
        ('echo %*>>"{0}"' -f $stripLog),
        'exit /b 0'
    )
    Set-Content -LiteralPath $fakeRanlib -Value @(
        '@echo off',
        ('echo %*>>"{0}"' -f $ranlibLog),
        'exit /b 0'
    )

    New-WirelessCustomerStage `
        -WorkspaceRoot $fixtureRoot `
        -StageRoot $stageRoot `
        -CustomerStripTool $fakeStrip `
        -CustomerRanlibTool $fakeRanlib | Out-Null
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a')) -Message 'WiFi library staged'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h')) -Message 'WiFi application header staged'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a')) -Message 'xStack library staged'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h')) -Message 'xStack application header staged'
    Assert-True -Condition (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'gplib\liba_wifi_src\atbm\atbm6165\include\wifi.h'))) -Message 'private WiFi header not staged'
    Assert-True -Condition (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'btstack\src\btstack.h'))) -Message 'internal BTstack header not staged'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'application\keep.c')) -Message 'non-wireless source preserved'
    Assert-Equal -Expected 0 -Actual @(
        Get-ChildItem -LiteralPath $stageRoot -Recurse -File -Filter '*.c' |
            Where-Object { $_.FullName -match '(atbm6165|btstack|atbm_rtos_wifi6)' }
    ).Count -Message 'no wireless C sources in stage'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'patches\wireless_static_library_integration.patch')) -Message 'integration patch staged'
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $stageRoot 'SHA256SUMS.txt')) -Message 'checksum manifest staged'
    Assert-True -Condition (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'Project\WirelessLibraries'))) -Message 'internal wireless library projects excluded'
    $stripCommands = @(Get-Content -LiteralPath $stripLog)
    $ranlibCommands = @(Get-Content -LiteralPath $ranlibLog)
    Assert-Equal -Expected 2 -Actual $stripCommands.Count -Message 'both customer archives stripped'
    Assert-True -Condition ($stripCommands[0] -match '^--strip-debug ') -Message 'strip removes debug information'
    Assert-Equal -Expected 2 -Actual $ranlibCommands.Count -Message 'both stripped archives reindexed'
    Assert-True -Condition ($ranlibCommands[0] -match '^-D ') -Message 'ranlib uses deterministic index'
}

$minimalFixture = Join-Path $workspace (
    'output\wireless_libs\test-minimal-stage-fixture-' + [guid]::NewGuid().ToString('N')
)
$minimalStage = Join-Path $minimalFixture 'stage'
foreach ($directory in @(
    'Project\App',
    'Project\Setup',
    'src',
    'inc',
    'gplib\liba_wifi_src\atbm\atbm6165\api',
    'gplib\liba_wifi_src\atbm\atbm6165\bt',
    'gplib\liba_wifi_src\atbm\atbm6165\hal\include',
    'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib',
    'gplib\liba_wifi_src\atbm\network\dhcp',
    'gplib\liba_wifi_src\atbm\network\include',
    'btstack\src',
    'fake-tools'
)) {
    New-Item -ItemType Directory -Path (Join-Path $minimalFixture $directory) -Force | Out-Null
}
$minimalProject = Join-Path $minimalFixture 'Project\App\app.gpcmprj'
$minimalProjectXml = @'
<emIDE_project_file>
  <Project>
    <Build>
      <Target title="Debug">
        <Linker>
          <Add option="-T../Setup/test.ld" />
          <Add library="../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a" />
          <Add library="../../gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a" />
        </Linker>
      </Target>
    </Build>
    <Unit filename="../../src/active.c"><Option compilerVar="CC" /></Unit>
    <!-- <Unit filename="../../btstack/src/hidden.c"><Option compilerVar="CC" /></Unit> -->
  </Project>
</emIDE_project_file>
'@
Set-Content -LiteralPath $minimalProject -Value $minimalProjectXml
Set-Content -LiteralPath (Join-Path $minimalFixture 'Project\Setup\test.ld') -Value 'SECTIONS {}'
Set-Content -LiteralPath (Join-Path $minimalFixture 'src\active.c') -Value '#include "used.h"'
$minimalUsedHeader = Join-Path $minimalFixture 'inc\used.h'
Set-Content -LiteralPath $minimalUsedHeader -Value '#define USED_HEADER 1'
Set-Content -LiteralPath (Join-Path $minimalFixture 'inc\unused.h') -Value '#define UNUSED_HEADER 1'
Set-Content -LiteralPath (Join-Path $minimalFixture 'btstack\src\hidden.c') -Value 'void hidden(void) {}'
$minimalApiHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi.h'
$minimalWifiDeliveryHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h'
$minimalBtDeliveryHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h'
$minimalUnusedApiHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_events.h'
$minimalTypesHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi_types.h'
$minimalDriverHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi_driver_api.h'
$minimalRawApiHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_api.h'
$minimalPrivateHeader = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\hal\include\atbm_hal.h'
Set-Content -LiteralPath $minimalApiHeader -Value '#define ATBM_PUBLIC_API 1'
Set-Content -LiteralPath $minimalWifiDeliveryHeader -Value '#define ATBM_WIFI_PUBLIC_API 1'
Set-Content -LiteralPath $minimalBtDeliveryHeader -Value '#define BT_XSTACK_PUBLIC_API 1'
Set-Content -LiteralPath $minimalUnusedApiHeader -Value '#define ATBM_PUBLIC_EVENTS_API 1'
Set-Content -LiteralPath $minimalTypesHeader -Value '#define ATBM_INTERNAL_TYPES 1'
Set-Content -LiteralPath $minimalDriverHeader -Value '#define ATBM_RAW_DRIVER_API 1'
Set-Content -LiteralPath $minimalRawApiHeader -Value '#define ATBM_RAW_API 1'
Set-Content -LiteralPath $minimalPrivateHeader -Value '#define ATBM_PRIVATE_API 1'
Set-Content -LiteralPath (Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\network\dhcp\dhcps.c') -Value 'void dhcps_start(void) {}'
Set-Content -LiteralPath (Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\network\include\lwipopts.h') -Value '#define LWIP_IPV4 1'
$minimalWlanLib = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib'
Set-Content -LiteralPath (Join-Path $minimalWlanLib 'libatbm_wifi.a') -Value 'wifi archive'
$minimalBtRoot = Join-Path $minimalFixture 'gplib\liba_wifi_src\atbm\atbm6165\bt'
Set-Content -LiteralPath (Join-Path $minimalBtRoot 'libxstack.a') -Value 'bt archive'
$minimalStripLog = Join-Path $minimalFixture 'strip.log'
$minimalRanlibLog = Join-Path $minimalFixture 'ranlib.log'
$minimalStrip = Join-Path $minimalFixture 'fake-tools\strip.cmd'
$minimalRanlib = Join-Path $minimalFixture 'fake-tools\ranlib.cmd'
Set-Content -LiteralPath $minimalStrip -Value @(
    '@echo off',
    ('echo %*>>"{0}"' -f $minimalStripLog),
    'exit /b 0'
)
Set-Content -LiteralPath $minimalRanlib -Value @(
    '@echo off',
    ('echo %*>>"{0}"' -f $minimalRanlibLog),
    'exit /b 0'
)

$minimalResult = New-MinimalWirelessCustomerStage `
    -WorkspaceRoot $minimalFixture `
    -StageRoot $minimalStage `
    -ProjectFile $minimalProject `
    -DependencyPaths @($minimalUsedHeader, $minimalWifiDeliveryHeader, $minimalBtDeliveryHeader) `
    -CustomerStripTool $minimalStrip `
    -CustomerRanlibTool $minimalRanlib
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'src\active.c')) -Message 'minimal stage keeps active source'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'inc\used.h')) -Message 'minimal stage keeps used header'
Assert-True -Condition (-not (Test-Path (Join-Path $minimalStage 'inc\unused.h'))) -Message 'minimal stage removes unused header'
Assert-True -Condition (-not (Test-Path (Join-Path $minimalStage 'btstack\src\hidden.c'))) -Message 'minimal stage removes wireless source'
Assert-True -Condition (-not (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165\api'))) -Message 'minimal stage omits source API directory'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h')) -Message 'minimal stage keeps WiFi application header'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h')) -Message 'minimal stage keeps xStack application header'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a')) -Message 'minimal stage keeps WiFi wlan_lib archive'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a')) -Message 'minimal stage keeps xStack bt archive'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\network\dhcp\dhcps.c')) -Message 'minimal stage keeps ATBM network sources'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\network\include\lwipopts.h')) -Message 'minimal stage keeps ATBM network headers'
Assert-Equal -Expected 2 -Actual @(
    Get-ChildItem -LiteralPath (Join-Path $minimalStage 'gplib\liba_wifi_src\atbm\atbm6165') -Directory
).Count -Message 'minimal stage exposes only bt and wlan_lib directories under ATBM'
Assert-Equal -Expected 2 -Actual @(Get-Content $minimalStripLog).Count -Message 'minimal stage strips wireless archives'
Assert-True -Condition (Test-Path (Join-Path $minimalStage 'SHA256SUMS.txt')) -Message 'minimal stage writes checksums'

$privateDependencyRejected = $false
try {
    New-MinimalWirelessCustomerStage `
        -WorkspaceRoot $minimalFixture `
        -StageRoot (Join-Path $minimalFixture 'private-stage') `
        -ProjectFile $minimalProject `
        -DependencyPaths @($minimalUsedHeader, $minimalPrivateHeader) | Out-Null
}
catch {
    $privateDependencyRejected = $_.Exception.Message -match 'non-delivery ATBM file'
}
Assert-True -Condition $privateDependencyRejected -Message 'minimal stage rejects private ATBM header dependencies'

$deliveryGuide = Get-Content -Raw -Encoding UTF8 -LiteralPath (
    Join-Path $workspaceRoot 'docs\wireless_static_library_integration.md')
Assert-True -Condition ($deliveryGuide -match 'bt/\{libxstack\.a,bt_xstack\.h\}') `
    -Message 'delivery guide documents the exact Bluetooth package files'
Assert-True -Condition ($deliveryGuide -match 'wlan_lib/\{libatbm_wifi\.a,atbm_wifi\.h\}') `
    -Message 'delivery guide documents the exact WiFi package files'
Assert-True -Condition ($deliveryGuide -notmatch 'ATBM 目录强制只保留 `api` 和 `wlan_lib`') `
    -Message 'delivery guide does not document the obsolete API package directory'

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: wireless library tool contract tests'
