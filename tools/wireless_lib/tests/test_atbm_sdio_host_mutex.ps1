$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$cardPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\source\atbm_gp_sdio_card.c'
$l2Path = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\source\atbm_gp_sdio_l2.c'
$headerPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio\include\atbm_gp_sdio_card.h'

$card = Get-Content -LiteralPath $cardPath -Raw
$l2 = Get-Content -LiteralPath $l2Path -Raw
$header = Get-Content -LiteralPath $headerPath -Raw

if ($card -notmatch 'atbm_gp_wifi_mutex\s*=\s*osMutexCreate\s*\(') {
    throw 'ATBM SDIO Host Mutex is never created'
}
if ($card -notmatch 'vSemaphoreDelete\s*\(\s*atbm_gp_wifi_mutex\s*\)') {
    throw 'ATBM SDIO Host Mutex is never deleted'
}
if ($header -notmatch 'atbm_gp_sdio_host_mutex_init') {
    throw 'ATBM SDIO Host Mutex lifecycle declarations are missing'
}

$mutexInit = $l2.IndexOf('atbm_gp_sdio_host_mutex_init()')
$irqEnable = $l2.IndexOf('vic_irq_enable(VIC_SD)')
if (($mutexInit -lt 0) -or ($mutexInit -gt $irqEnable)) {
    throw 'ATBM SDIO Host Mutex must be initialized before runtime IRQ enable'
}

$irqDisable = $l2.IndexOf('vic_irq_disable(VIC_SD)')
$mutexDeinit = $l2.IndexOf('atbm_gp_sdio_host_mutex_deinit()')
if (($irqDisable -lt 0) -or ($mutexDeinit -lt $irqDisable)) {
    throw 'ATBM SDIO Host Mutex must be deleted after runtime IRQ disable'
}

Write-Host 'ATBM SDIO Host Mutex tests passed.'
