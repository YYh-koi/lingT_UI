$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$atbmLibraryProject = Join-Path $workspace 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
$btstackLibraryProject = Join-Path $workspace 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'
$atbmMainProject = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$rtkMainProject = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj'
$networkRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\network'
$oldNetworkRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\network'

foreach ($required in @(
    $atbmLibraryProject,
    $btstackLibraryProject,
    $atbmMainProject,
    $rtkMainProject,
    (Join-Path $networkRoot 'include\lwipopts.h'),
    (Join-Path $networkRoot 'include\atbm_lwip_port.h'),
    (Join-Path $networkRoot 'port\atbm_lwip_port.c'),
    (Join-Path $networkRoot 'dhcp\dhcps.c'),
    (Join-Path $networkRoot 'dhcp\dhcps.h'),
    (Join-Path $networkRoot 'lwip_v2.1.3\ports\freertos\sys_arch.c'),
    (Join-Path $networkRoot 'lwip_v2.1.3\src\api\tcpip.c')
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing required ATBM isolation artifact: $required"
    }
}

if (Test-Path -LiteralPath $oldNetworkRoot) {
    throw "Legacy nested ATBM network directory still exists: $oldNetworkRoot"
}

$libraryText = Get-Content -LiteralPath $atbmLibraryProject -Raw
$btstackText = Get-Content -LiteralPath $btstackLibraryProject -Raw
$mainText = Get-Content -LiteralPath $atbmMainProject -Raw
$rtkText = Get-Content -LiteralPath $rtkMainProject -Raw

$oldNetworkPattern = 'liba_wifi_src[/\\]atbm[/\\]atbm6165[/\\]platform[/\\]generalplus[/\\]network'
$newNetworkPattern = 'liba_wifi_src[/\\]atbm[/\\]network'
foreach ($activeProjectText in @($libraryText, $btstackText, $mainText)) {
    if ($activeProjectText -match $oldNetworkPattern) {
        throw 'Active project still references the legacy nested ATBM network path'
    }
}
if ($libraryText -notmatch $newNetworkPattern) {
    throw 'ATBM library project does not include headers from atbm/network'
}
if ($btstackText -notmatch $newNetworkPattern) {
    throw 'BTstack library project does not include headers from atbm/network'
}
if ($libraryText -match '<Unit\s+filename="[^"]*liba_wifi_src[/\\]atbm[/\\]network[/\\][^"]*\.c"') {
    throw 'ATBM library project must not compile network source units into libatbm_wifi.a'
}
if ($mainText -notmatch '<Unit\s+filename="[^"]*liba_wifi_src[/\\]atbm[/\\]network[/\\]dhcp[/\\]dhcps\.c"') {
    throw 'ATBM main project does not compile the customer-delivered DHCP server'
}
if ($mainText -notmatch '<Unit\s+filename="[^"]*liba_wifi_src[/\\]atbm[/\\]network[/\\]port[/\\]atbm_lwip_port\.c"') {
    throw 'ATBM main project does not compile the ATBM lwIP port'
}

foreach ($atbmText in @($libraryText, $mainText)) {
    if ($atbmText -match 'liba_wifi_src[/\\]rtk') {
        throw 'ATBM project still references liba_wifi_src/rtk'
    }
}
if ($mainText -notmatch 'WIFI_VENDOR=3') { throw 'ATBM main project must define WIFI_VENDOR=3' }
if ($mainText -notmatch 'libatbm_wifi\.a') { throw 'ATBM main project must link libatbm_wifi.a' }
if ($mainText -match 'lib8189FTV_WLAN\.a') { throw 'ATBM main project must not link lib8189FTV_WLAN.a' }
if ($mainText -match 'rtk_wifi_demo') { throw 'ATBM main project must not compile an RTK WiFi demo' }
if ($rtkText -notmatch 'WIFI_VENDOR=2') { throw '8189FTV project must define WIFI_VENDOR=2' }
if ($rtkText -notmatch 'liba_wifi_src[/\\]rtk') { throw '8189FTV project must retain RTK sources' }
if ($rtkText -notmatch 'lib8189FTV_WLAN\.a') { throw '8189FTV project must link lib8189FTV_WLAN.a' }
if ($rtkText -match 'libatbm_wifi\.a') { throw '8189FTV project must not link libatbm_wifi.a' }
if ($rtkText -match 'atbm_wifi_probe_demo') { throw '8189FTV project must not compile the ATBM WiFi demo' }

$interfacePath = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wifi_interface.c'
$interfaceText = Get-Content -LiteralPath $interfacePath -Raw
if ($interfaceText -match '#\s*include\s*["<]lwip_netconf\.h[">]') {
    throw 'ATBM interface still includes RTK lwip_netconf.h'
}
if ($interfaceText -notmatch '\batbm_lwip_system_init\s*\(') {
    throw 'ATBM interface does not use the ATBM lwIP port'
}

$dhcpsText = Get-Content -LiteralPath (Join-Path $networkRoot 'dhcp\dhcps.c') -Raw
if ($dhcpsText -match '#\s*include\s*["<]wlan_setting\.h[">]') {
    throw 'ATBM DHCP server still includes RTK wlan_setting.h'
}
if ($dhcpsText -match '\b(?:GP_AUTH_IP_ADDR[0-3]|g_ap_sta_num)\b') {
    throw 'ATBM DHCP server still depends on RTK AP configuration symbols'
}

$skbufPath = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\atbm_os_skbuf.c'
$skbufText = Get-Content -LiteralPath $skbufPath -Raw
if ($skbufText -notmatch 'ucIPAddress\s*\[\s*4\s*\]\s*=\s*\{\s*192\s*,\s*168\s*,\s*42\s*,\s*1\s*\}') {
    throw 'ATBM AP default IP must remain 192.168.42.1'
}

Write-Host 'PASS: ATBM delivery layout and RTK build dependencies are isolated'
