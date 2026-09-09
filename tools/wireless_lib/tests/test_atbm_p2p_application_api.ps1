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
$driverHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi_driver_api.h'
$driverSourcePath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wifi_driver_api.c'
$interfaceSourcePath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wifi_interface.c'
$p2pMainSourcePath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\net\wpa\p2p\p2p_main.c'
$halHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\include\atbm_hal.h'
$demoPath = Join-Path $workspace 'Project\wifi_demo\atbm_wifi_probe_demo.c'

foreach ($path in @(
    $driverHeaderPath,
    $driverSourcePath,
    $interfaceSourcePath,
    $p2pMainSourcePath,
    $halHeaderPath,
    $demoPath
)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing required source: $path"
    }
}

$driverHeader = Get-Content -LiteralPath $driverHeaderPath -Raw
$driverSource = Get-Content -LiteralPath $driverSourcePath -Raw
$interfaceSource = Get-Content -LiteralPath $interfaceSourcePath -Raw
$p2pMainSource = Get-Content -LiteralPath $p2pMainSourcePath -Raw
$halHeader = Get-Content -LiteralPath $halHeaderPath -Raw
$demoSource = Get-Content -LiteralPath $demoPath -Raw

foreach ($name in @(
    'atbm_wifi_get_associated_client_list_vif',
    'atbm_wifi_get_client_rssi_vif',
    'atbm_wifi_disconnect_client_vif',
    'atbm_wifi_p2p_go_stop'
)) {
    Assert-Match `
        -Text $driverHeader `
        -Pattern ("\b{0}\s*\(" -f [regex]::Escape($name)) `
        -Message "$name is declared by the private driver API"
}

foreach ($name in @(
    'atbm_wifi_is_connected',
    'atbm_wifi_get_connection_info',
    'atbm_wifi_get_rssi',
    'atbm_wifi_get_ip_info',
    'atbm_wifi_get_clients',
    'atbm_wifi_disconnect_client'
)) {
    Assert-Match `
        -Text $interfaceSource `
        -Pattern ("\b{0}\s*\(" -f [regex]::Escape($name)) `
        -Message "$name is implemented by the public interface"
}

Assert-Match `
    -Text $driverSource `
    -Pattern 'if\s*\(priv\s*==\s*ATBM_NULL\)[\s\S]{0,240}g_hw_prv\.p2p_if_id\s*=\s*priv->if_id' `
    -Message 'P2P start validates priv before dereference'
Assert-NotMatch `
    -Text $driverSource `
    -Pattern 'g_hw_prv\.p2p_if_id\s*=\s*priv->if_id;[\s\S]{0,240}if\s*\(priv\s*==\s*ATBM_NULL\)' `
    -Message 'P2P start has no pre-check dereference'
Assert-Match `
    -Text $driverSource `
    -Pattern 'ret\s*=\s*atbm_p2p_go_start\s*\(priv\)' `
    -Message 'P2P GO start propagates the lower-layer result'
Assert-Match `
    -Text $driverSource `
    -Pattern 'capacity\s*=\s*maclist->count;[\s\S]{0,420}maclist->count\s*=\s*capacity' `
    -Message 'client-list capacity survives output clearing'
Assert-Match `
    -Text $driverHeader `
    -Pattern 'pwd\s*\[\s*ATBM_P2P_PASSWORD_MAX_LEN\s*\+\s*1\s*\]' `
    -Message 'P2P password storage accepts 63 bytes plus terminator'
Assert-Match `
    -Text $interfaceSource `
    -Pattern 'ATBM_WIFI_ASSOCRSP_TXOK_EVENT[\s\S]{0,220}ATBM_WIFI_EVENT_CLIENT_CONNECTED' `
    -Message 'AP/P2P client-connect event is forwarded'
Assert-Match `
    -Text $interfaceSource `
    -Pattern 'ATBM_WIFI_MODE_P2P_GO[\s\S]{0,260}ATBM_WIFI_EVENT_CLIENT_DISCONNECTED' `
    -Message 'P2P GO client-disconnect event is forwarded'

Assert-Match `
    -Text $demoSource `
    -Pattern '\batbm_wifi_start_p2p\s*\(' `
    -Message 'application demo exposes a public P2P GO example'
Assert-NotMatch `
    -Text $demoSource `
    -Pattern '#\s*include\s*[<"](?:atbm_wifi_driver_api|atbm_hal|p2p_main)' `
    -Message 'application demo has no private ATBM include'
Assert-NotMatch `
    -Text $demoSource `
    -Pattern '(?:passwd|password)\s*=\s*%s' `
    -Message 'application demo does not log a WiFi password'
Assert-NotMatch `
    -Text $p2pMainSource `
    -Pattern '(?:pwd|passwd|password)\s*[:=]\s*%s' `
    -Message 'P2P GO startup does not log a WiFi password'

Assert-Match `
    -Text $halHeader `
    -Pattern '\bp2p_init_event\b[\s\S]{0,160}\bp2p_init_result\b' `
    -Message 'P2P VIF stores initialization completion state'
Assert-Match `
    -Text $p2pMainSource `
    -Pattern 'p2p_init_result\s*=\s*atbm_p2p_init\s*\([^;]+;[\s\S]{0,180}atbm_os_wakeup_event\s*\(\s*&priv->p2p_init_event\s*\)' `
    -Message 'P2P task records and signals initialization completion'
Assert-Match `
    -Text $p2pMainSource `
    -Pattern 'int\s+atbm_p2p_start\s*\([^\)]*\)[\s\S]{0,1000}atbm_os_wait_event_timeout\s*\(\s*&priv->p2p_init_event' `
    -Message 'P2P start waits for task initialization before returning'
Assert-Match `
    -Text $p2pMainSource `
    -Pattern 'atbm_os_wait_event_timeout\s*\(\s*&priv->p2p_init_event[\s\S]{0,420}p2p_init_result' `
    -Message 'P2P start checks the task initialization result'

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: ATBM P2P and application API contract'
