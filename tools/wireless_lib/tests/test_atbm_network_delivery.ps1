$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$modulePath = Join-Path $workspace 'tools\wireless_lib\WirelessLibTools.psm1'
Import-Module $modulePath -Force

$fixtureRoot = Join-Path $workspace ('output\wireless_libs\test-atbm-network-delivery-' + [guid]::NewGuid().ToString('N'))
$stageRoot = Join-Path $fixtureRoot 'stage'

try {
    foreach ($directory in @(
        'Project\App',
        'src',
        'Setup',
        'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib',
        'gplib\liba_wifi_src\atbm\atbm6165\bt',
        'gplib\liba_wifi_src\atbm\network\dhcp',
        'gplib\liba_wifi_src\atbm\network\include'
    )) {
        New-Item -ItemType Directory -Path (Join-Path $fixtureRoot $directory) -Force | Out-Null
    }

    $projectPath = Join-Path $fixtureRoot 'Project\App\app.gpcmprj'
    Set-Content -LiteralPath $projectPath -Value @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<emIDE_project_file>
  <Project>
    <Build>
      <Target title="Debug">
        <Linker>
          <Add option="-T../../Setup/test.ld" />
          <Add library="../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a" />
          <Add library="../../gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a" />
        </Linker>
      </Target>
    </Build>
    <Unit filename="../../src/active.c"><Option compilerVar="CC" /></Unit>
  </Project>
</emIDE_project_file>
'@
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'src\active.c') -Value 'void active(void) {}'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'Setup\test.ld') -Value 'SECTIONS {}'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a') -Value 'wifi archive'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h') -Value '#define ATBM_WIFI_PUBLIC 1'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a') -Value 'bt archive'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h') -Value '#define BT_XSTACK_PUBLIC 1'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\network\dhcp\dhcps.c') -Value 'void dhcps_start(void) {}'
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'gplib\liba_wifi_src\atbm\network\include\lwipopts.h') -Value '#define LWIP_IPV4 1'

    New-MinimalWirelessCustomerStage `
        -WorkspaceRoot $fixtureRoot `
        -StageRoot $stageRoot `
        -ProjectFile $projectPath `
        -DependencyPaths @() | Out-Null

    foreach ($relativePath in @(
        'gplib\liba_wifi_src\atbm\network\dhcp\dhcps.c',
        'gplib\liba_wifi_src\atbm\network\include\lwipopts.h'
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $relativePath) -PathType Leaf)) {
            throw "Customer package is missing ATBM network file: $relativePath"
        }
    }

    Write-Host 'PASS: complete ATBM network tree is delivered to customers'
}
finally {
    $fixtureParent = (Resolve-Path -LiteralPath (Join-Path $workspace 'output\wireless_libs')).Path.TrimEnd('\')
    if ((Test-Path -LiteralPath $fixtureRoot) -and
        $fixtureRoot.StartsWith($fixtureParent + '\', [StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
