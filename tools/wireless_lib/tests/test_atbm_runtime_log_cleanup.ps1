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
$configPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\include\atbm_config.h'
$cardPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\source\atbm_gp_sdio_card.c'
$sdioPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\sdio\atbm_sdio.c'
$apPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_ap.c'
$wsmPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wsm.c'
$txrxPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_txrx.c'
$mgmtPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\net\atbm_mgmt.c'
$halPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\include\atbm_hal.h'
$irqHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\core\atbm_gp_sdio_irq.h'
$cardHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\include\atbm_gp_sdio_card.h'

$config = Get-Content -LiteralPath $configPath -Raw
$card = Get-Content -LiteralPath $cardPath -Raw
$sdio = Get-Content -LiteralPath $sdioPath -Raw
$ap = Get-Content -LiteralPath $apPath -Raw
$wsm = Get-Content -LiteralPath $wsmPath -Raw
$txrx = Get-Content -LiteralPath $txrxPath -Raw
$mgmt = Get-Content -LiteralPath $mgmtPath -Raw
$hal = Get-Content -LiteralPath $halPath -Raw
$irqHeader = Get-Content -LiteralPath $irqHeaderPath -Raw
$cardHeader = Get-Content -LiteralPath $cardHeaderPath -Raw

$diagnosticSources = $config + $card + $sdio + $ap + $wsm + $txrx + $mgmt +
    $hal + $irqHeader + $cardHeader

foreach ($temporaryPattern in @(
    'ATBM_AP_RX_DIAG',
    'ATBM_AP_LINK_DIAG',
    '\[APDIAG\]',
    '\[AP\] rxdiag',
    'atbmwifi_ap_link_diag_timeout',
    'atbm_gp_sdio_irq_diag_dump',
    'atbm_gp_sdio_rx_timeout_diag',
    'ATBM_SDIO_RX_STAGE_',
    'ATBM_SDIO_RX_\w*DIAG',
    '\bdiag_sdio',
    '\bdiag_wsm',
    '\bdiag_rx_cb',
    '\bdiag_ap_mgmt',
    '\bdiag_rx_last'
)) {
    Assert-NotMatch `
        -Text $diagnosticSources `
        -Pattern $temporaryPattern `
        -Message "temporary AP/SDIO diagnostic source removed: $temporaryPattern"
}

Assert-Match `
    -Text $config `
    -Pattern '#define\s+ATBM_AP_STALE_WATCHDOG\b' `
    -Message 'AP stale watchdog configuration retained'
Assert-Match `
    -Text $ap `
    -Pattern 'atbmwifi_ap_stale_timeout\s*\(' `
    -Message 'AP stale watchdog implementation retained'

foreach ($temporaryPattern in @(
    '\[SDIO-IRQ\]',
    '\[SDIO-RX-RECOVER\]',
    'atbm_gp_sdio_irq_diag_print',
    'atbm_gp_sdio_irq_diag_dump',
    'atbm_gp_sdio_rx_timeout_diag',
    'atbm_sdio_irq_isr_count',
    'atbm_sdio_irq_thread_count',
    'create counting semaphore',
    'create irq thread'
)) {
    Assert-NotMatch `
        -Text $card `
        -Pattern $temporaryPattern `
        -Message "temporary SDIO diagnostic removed: $temporaryPattern"
}

foreach ($functionalPattern in @(
    'xSemaphoreCreateCounting\s*\(',
    'xSemaphoreGiveFromISR\s*\(',
    'xSemaphoreTake\s*\(',
    'atbm_gp_sdio_host_enable_isr\s*\(false\)',
    'atbm_gp_wifi_sdio_func->irq_handler\s*\(',
    'atbm_sdio_rx_irq_reenable\s*\(hw_priv\)',
    'atbm_light_sleep_try_to_keepalive\s*\(hw_priv,\s*ATBM_SDIO_WAKEUP_REASON_RX_PROGRESS\)'
)) {
    $source = if ($functionalPattern -like 'atbm_sdio_*' -or
        $functionalPattern -like 'atbm_light_sleep*') { $sdio } else { $card }
    Assert-Match `
        -Text $source `
        -Pattern $functionalPattern `
        -Message "functional SDIO/RX path retained: $functionalPattern"
}

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: ATBM temporary runtime diagnostics removed'
