$ErrorActionPreference = 'Stop'
$script:Failures = 0

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

function Assert-NotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -match $Pattern) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspace = (Resolve-Path (Join-Path $testDirectory '..\..\..')).Path
$headerPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi.h'

if (-not (Test-Path -LiteralPath $headerPath -PathType Leaf)) {
    throw "Missing public WiFi interface header: $headerPath"
}

$headerText = Get-Content -LiteralPath $headerPath -Raw
$deliveryHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h'
if (-not (Test-Path -LiteralPath $deliveryHeaderPath -PathType Leaf)) {
    throw "Missing delivered WiFi interface header: $deliveryHeaderPath"
}
$deliveryHeaderText = Get-Content -LiteralPath $deliveryHeaderPath -Raw

$netifInitPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\atbm_os_skbuf.c'
if (-not (Test-Path -LiteralPath $netifInitPath -PathType Leaf)) {
    throw "Missing ATBM netif initialization source: $netifInitPath"
}
$netifInitText = Get-Content -LiteralPath $netifInitPath -Raw

foreach ($networkHeader in @($headerText, $deliveryHeaderText)) {
    Assert-NotMatch `
        -Text $networkHeader `
        -Pattern '\bATBM_WIFI_SERVER_(?:IP|NETMASK|GATEWAY)_ADDR[0-3]\b' `
        -Message 'public atbm_wifi.h does not expose internal server address macros'
}

Assert-Match `
    -Text $netifInitText `
    -Pattern 'ucIPAddress\s*\[\s*4\s*\]\s*=\s*\{\s*192\s*,\s*168\s*,\s*42\s*,\s*1\s*\}' `
    -Message 'netif initialization owns default address 192.168.42.1'
Assert-Match `
    -Text $netifInitText `
    -Pattern 'ucNetMask\s*\[\s*4\s*\]\s*=\s*\{\s*255\s*,\s*255\s*,\s*255\s*,\s*0\s*\}' `
    -Message 'netif initialization owns default netmask 255.255.255.0'
Assert-Match `
    -Text $netifInitText `
    -Pattern 'netif_add\s*\(\s*p_netif\s*,\s*&ip_addr\s*,\s*&netmask\s*,\s*&gw' `
    -Message 'netif_add receives the default address during initialization'

if ($headerText -cne $deliveryHeaderText) {
    $script:Failures++
    Write-Host 'FAIL: API and delivered atbm_wifi.h copies are identical' -ForegroundColor Red
}

foreach ($typeName in @(
    'atbm_wifi_security_t',
    'atbm_wifi_interface_mode_t',
    'atbm_wifi_state_t',
    'atbm_wifi_error_t',
    'atbm_wifi_ap_config_t',
    'atbm_wifi_sta_config_t',
    'atbm_wifi_p2p_config_t',
    'atbm_wifi_scan_result_t',
    'atbm_wifi_status_t',
    'atbm_wifi_connection_info_t',
    'atbm_wifi_ip_info_t',
    'atbm_wifi_client_info_t',
    'atbm_wifi_event_t',
    'atbm_wifi_event_info_t',
    'atbm_wifi_event_callback_t'
)) {
    Assert-Match `
        -Text $headerText `
        -Pattern ("\b{0}\b" -f [regex]::Escape($typeName)) `
        -Message "$typeName is declared"
}

foreach ($functionName in @(
    'atbm_wifi_init',
    'atbm_wifi_start_ap',
    'atbm_wifi_stop_ap',
    'atbm_wifi_start_sta',
    'atbm_wifi_stop_sta',
    'atbm_wifi_start_p2p',
    'atbm_wifi_stop_p2p',
    'atbm_wifi_scan',
    'atbm_wifi_get_status',
    'atbm_wifi_get_mac',
    'atbm_wifi_is_connected',
    'atbm_wifi_get_connection_info',
    'atbm_wifi_get_rssi',
    'atbm_wifi_get_ip_info',
    'atbm_wifi_get_clients',
    'atbm_wifi_disconnect_client',
    'atbm_wifi_register_event_callback',
    'atbm_wifi_deinit'
)) {
    Assert-Match `
        -Text $headerText `
        -Pattern ("\b{0}\s*\(" -f [regex]::Escape($functionName)) `
        -Message "$functionName is declared"
}

$interfacePath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wifi_interface.c'
$mainPath = Join-Path $workspace `
    'Project\GPCV2178_Turnkey_SDK\src\main.c'
foreach ($path in @($interfacePath, $mainPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing synchronous WiFi lifecycle source: $path"
    }
}

$interfaceText = Get-Content -LiteralPath $interfacePath -Raw
$mainText = Get-Content -LiteralPath $mainPath -Raw

Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '#\s*include\s*"wlan_setting\.h"' `
    -Message 'ATBM interface does not include 8189 wlan_setting.h'

foreach ($legacyMacro in @(
    'GP_AUTH_IP_ADDR',
    'NETMASK_ADDR',
    'GW_ADDR'
)) {
    Assert-NotMatch `
        -Text $interfaceText `
        -Pattern ("\b{0}[0-3]\b" -f $legacyMacro) `
        -Message "ATBM interface does not use legacy $legacyMacro macros"
}

Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '\bnetif_set_addr\s*\(' `
    -Message 'network server startup does not override the initialized netif address'
Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '\bATBM_WIFI_SERVER_' `
    -Message 'network server startup does not consume public address macros'

Assert-Match `
    -Text $interfaceText `
    -Pattern 'atbm_wifi_network_server_start_locked[\s\S]{0,2500}netif_set_default\s*\([\s\S]{0,300}netif_set_link_up\s*\([\s\S]{0,300}netif_set_up\s*\([\s\S]{0,500}dhcps_init\s*\(\s*netif\s*\)' `
    -Message 'network server startup still enables the netif and starts DHCP Server'

Assert-Match `
    -Text $interfaceText `
    -Pattern 'int\s+atbm_wifi_init\s*\(\s*void\s*\)[\s\S]{0,500}atbm_wifi_platform_init_locked\s*\(' `
    -Message 'atbm_wifi_init synchronously performs common platform initialization'
Assert-Match `
    -Text $interfaceText `
    -Pattern 'static\s+int\s+atbm_wifi_prepare_start_locked[\s\S]{0,500}driver_ready\s*==\s*ATBM_FALSE[\s\S]{0,180}ATBM_WIFI_ERR_NOT_READY' `
    -Message 'mode start requires successful explicit initialization'
Assert-NotMatch `
    -Text $interfaceText `
    -Pattern 'static\s+int\s+atbm_wifi_prepare_start_locked[\s\S]{0,900}atbm_wifi_platform_init_locked\s*\(' `
    -Message 'mode preparation does not implicitly initialize the platform'
Assert-Match `
    -Text $mainText `
    -Pattern 'atbm_wifi_init\s*\(\s*\)[\s\S]{0,500}atbm_wifi_p2p_mode_enable\s*\(' `
    -Message 'application initializes WiFi before starting P2P mode'

$expectedErrors = [ordered]@{
    ATBM_WIFI_OK = 0
    ATBM_WIFI_ERR_INVALID_ARG = -1
    ATBM_WIFI_ERR_INVALID_STATE = -2
    ATBM_WIFI_ERR_PLATFORM_INIT = -3
    ATBM_WIFI_ERR_DRIVER_INIT = -4
    ATBM_WIFI_ERR_START_FAILED = -5
    ATBM_WIFI_ERR_NETWORK_FAILED = -6
    ATBM_WIFI_ERR_NO_MEMORY = -7
    ATBM_WIFI_ERR_NOT_READY = -8
}
foreach ($entry in $expectedErrors.GetEnumerator()) {
    Assert-Match `
        -Text $headerText `
        -Pattern ("{0}\s*=\s*{1}\b" -f [regex]::Escape($entry.Key), $entry.Value) `
        -Message "$($entry.Key) has stable value $($entry.Value)"
}

Assert-NotMatch `
    -Text $headerText `
    -Pattern '#\s*include\s*[<"](?:atbm_wifi_interface|atbm_wifi_types|atbm_hal|atbm_wifi_driver_api|atbm_api|lwip|[^">]*sdio|FreeRTOS)' `
    -Message 'public header has no private driver, network, SDIO, or RTOS includes'

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: ATBM WiFi interface public contract'
