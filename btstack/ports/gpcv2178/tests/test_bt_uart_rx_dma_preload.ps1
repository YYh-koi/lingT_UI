$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$uartPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_uart_gpcv2178.c'

$config = Get-Content -Raw -LiteralPath $configPath
$uart = Get-Content -Raw -LiteralPath $uartPath

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE\s+512u') {
    throw 'UART RX DMA block size must remain 512 bytes'
}
foreach ($state in @(
        'btstack_uart_h5_rx_dma_channel_owned',
        'btstack_uart_h5_rx_dma_queued_block')) {
    if ($uart -notmatch "static\s+volatile\s+uint8_t\s+$state") {
        throw "Missing preloaded double-buffer state: $state"
    }
}

$preload = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_h5_rx_dma_try_preload\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $preload.Success) {
    throw 'Missing task-context DMA preload helper'
}
if (($preload.Value -notmatch 'drv_l1_dma_transfer_double_buf_set') -or
    ($preload.Value -notmatch 'btstack_uart_h5_rx_dma_queued_block\s*=\s*\(uint8_t\)block')) {
    throw 'DMA preload helper does not fill and track the hardware queued block'
}

$arm = [regex]::Match(
    $uart,
    '(?s)static\s+int\s+btstack_uart_h5_rx_dma_arm_new\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $arm.Success) {
    throw 'Missing new-channel DMA arm helper'
}
$runningIndex = $arm.Value.IndexOf('btstack_uart_h5_rx_dma_running = 1')
$preloadIndex = $arm.Value.IndexOf('btstack_uart_h5_rx_dma_try_preload();')
if (($runningIndex -lt 0) -or ($preloadIndex -lt 0) -or ($preloadIndex -lt $runningIndex)) {
    throw 'Startup must preload a second block after starting the first block'
}

$isr = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_gpcv2178_rx_dma_isr\s*\([^;]*?\)\s*\{.*?\n\}')
if (-not $isr.Success) {
    throw 'Missing UART RX DMA ISR implementation'
}
if ($isr.Value -match 'drv_l1_dma_transfer_double_buf_set|drv_l1_dma_transfer_double_buf_free|btstack_uart_h5_rx_dma_arm_new') {
    throw 'UART RX DMA ISR must not call DMA allocation, preload, or release APIs'
}
if (($isr.Value -notmatch 'btstack_uart_h5_rx_dma_active_block\s*=\s*btstack_uart_h5_rx_dma_queued_block') -or
    ($isr.Value -notmatch 'btstack_uart_h5_rx_dma_queued_block\s*=\s*0xffu') -or
    ($isr.Value -notmatch 'btstack_uart_h5_rx_dma_reload_pending\s*=\s*1u')) {
    throw 'UART RX DMA ISR does not promote the preloaded block and request another preload'
}

$idleFlush = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_h5_rx_flush_idle_dma\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $idleFlush.Success -or
    $idleFlush.Value -notmatch 'btstack_uart_h5_rx_dma_release_queued_block') {
    throw 'Idle flush does not release the unused preloaded block'
}

$close = [regex]::Match(
    $uart,
    '(?s)static\s+int\s+btstack_uart_gpcv2178_close\s*\([^;]*?\)\s*\{.*?\n\}')
if (-not $close.Success -or
    $close.Value -notmatch 'btstack_uart_h5_rx_dma_release_channel' -or
    $close.Value -notmatch 'btstack_uart_h5_rx_dma_release_queued_block') {
    throw 'Close path does not release channel and queued-block ownership'
}

foreach ($field in @('owned=%u', 'queued=%u', 'dbf=%d', 'preload=%u')) {
    if ($uart -notmatch [regex]::Escape($field)) {
        throw "UART health diagnostics are missing: $field"
    }
}

Write-Host 'UART RX DMA uses an active block plus a hardware-preloaded queued block'
