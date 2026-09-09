$ErrorActionPreference = 'Stop'

$workspace = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$configPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h'
$cardPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c'
$halPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h'
$sdioPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/hal/sdio/atbm_sdio.c'
$threadPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_thread.c'
$apPath = Join-Path $workspace 'gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_ap.c'

$config = Get-Content -Raw -LiteralPath $configPath
$card = Get-Content -Raw -LiteralPath $cardPath
$hal = Get-Content -Raw -LiteralPath $halPath
$sdio = Get-Content -Raw -LiteralPath $sdioPath
$thread = Get-Content -Raw -LiteralPath $threadPath
$ap = Get-Content -Raw -LiteralPath $apPath
$failures = @()

if ($config -notmatch '(?m)^#define\s+ATBM_AP_RX_DIAG\s+1\s*$') {
    $failures += 'ATBM_AP_RX_DIAG is not enabled'
}
if ($config -notmatch '(?m)^#define\s+ATBM_AP_LINK_DIAG\s+1\s*$') {
    $failures += 'ATBM_AP_LINK_DIAG is not enabled'
}
if ($card -notmatch '(?m)^#define\s+ATBM_SDIO_IRQ_DIAG_TRACE_LIMIT\s+8\s*$') {
    $failures += 'ATBM_SDIO_IRQ_DIAG_TRACE_LIMIT is not 8'
}
if ($card -notmatch '\[SDIO-IRQ\]') {
    $failures += '[SDIO-IRQ] summary marker is missing'
}
foreach ($counter in @(
    'atbm_sdio_irq_isr_count',
    'atbm_sdio_irq_put_ok_count',
    'atbm_sdio_irq_put_fail_count',
    'atbm_sdio_irq_thread_count',
    'atbm_sdio_irq_handler_count',
    'atbm_sdio_irq_zero_pending_count',
    'atbm_sdio_irq_read_fail_count'
)) {
    if ($card -notmatch [regex]::Escape($counter)) {
        $failures += "$counter is missing from GP SDIO IRQ diagnostics"
    }
}
if ($card -notmatch 'put_ret\s*=\s*osMessagePut') {
    $failures += 'SDIO ISR queue result is not checked'
}
if ($card -notmatch 'atbm_sdio_irq_put_fail_count\+\+;[\s\S]{0,160}?atbm_gp_sdio_host_enable_isr\(true\)') {
    $failures += 'SDIO IRQ is not re-enabled after queue put failure'
}
if ($card -notmatch 'void\s+atbm_gp_sdio_irq_diag_dump\s*\(') {
    $failures += 'GP SDIO IRQ periodic dump is missing'
}
if ($sdio -notmatch '(?m)^#define\s+ATBM_SDIO_RX_FALLBACK_MS\s+100\s*$') {
    $failures += 'Atbm_RX fallback is not 100 ms'
}
if ($sdio -notmatch 'RX_BH_TASK_PRIO,\s*\r?\n\s*ATBM_SDIO_RX_FALLBACK_MS\);') {
    $failures += 'Atbm_RX thread does not use ATBM_SDIO_RX_FALLBACK_MS'
}
if ($thread -notmatch 'int\s+ret\s*=\s*1\s*;') {
    $failures += 'thread wake reason is not initialized to IRQ/event wake'
}
foreach ($field in @(
    'diag_sdio_rx_task_enter_count',
    'diag_sdio_rx_task_exit_count',
    'diag_sdio_irq_reenable_count',
    'diag_sdio_rx_stage',
    'diag_sdio_rx_ret',
    'diag_sdio_rx_irq_wake_count',
    'diag_sdio_rx_timeout_wake_count',
    'diag_sdio_rx_timeout_recovery_count',
    'diag_sdio_last_rx_len'
)) {
    if ($hal -notmatch [regex]::Escape($field)) {
        $failures += "$field is missing from atbmwifi_common"
    }
    if ($ap -notmatch [regex]::Escape($field)) {
        $failures += "$field is missing from APDIAG output"
    }
}

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output 'ATBM disconnect diagnostics source check passed'
