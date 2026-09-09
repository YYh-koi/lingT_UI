$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$uartPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_uart_gpcv2178.c'

foreach ($path in @($configPath, $uartPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing UART RX DMA deferred-reload artifact: $path"
    }
}

$config = Get-Content -Raw -LiteralPath $configPath
$uart = Get-Content -Raw -LiteralPath $uartPath

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE\s+512u') {
    throw 'UART RX DMA block size must remain 512 bytes'
}
if ($uart -notmatch 'static\s+volatile\s+uint8_t\s+btstack_uart_h5_rx_dma_reload_pending') {
    throw 'Missing dedicated UART RX DMA reload-pending state'
}

$isr = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_gpcv2178_rx_dma_isr\s*\([^;]*?\)\s*\{.*?\n\}')
if (-not $isr.Success) {
    throw 'Missing UART RX DMA ISR implementation'
}
if ($isr.Value -match 'drv_l1_dma_transfer_double_buf_set|drv_l1_dma_transfer_double_buf_free|btstack_uart_h5_rx_dma_arm_new') {
    throw 'UART RX DMA ISR still calls a DMA allocation, preload, or release API'
}
if (($isr.Value -notmatch 'btstack_uart_h5_rx_dma_active_block\s*=\s*btstack_uart_h5_rx_dma_queued_block') -or
    ($isr.Value -notmatch 'btstack_uart_h5_rx_dma_reload_pending\s*=\s*1u')) {
    throw 'UART RX DMA ISR does not rotate the preloaded block and defer the refill'
}

$reload = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_h5_rx_dma_try_preload\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $reload.Success) {
    throw 'Missing run-loop UART RX DMA preload helper'
}
foreach ($token in @(
        'drv_l1_dma_transfer_double_buf_set',
        'btstack_uart_h5_rx_dma_queued_block = (uint8_t)block',
        'btstack_uart_h5_rx_dma_reload_pending = 0u')) {
    if ($reload.Value -notmatch [regex]::Escape($token)) {
        throw "Deferred preload behavior is missing: $token"
    }
}

$process = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_h5_rx_process_ready\s*\([^;]*?\)\s*\{.*?\n\}')
if (-not $process.Success) {
    throw 'Missing UART H5 run-loop receive processor'
}
$reloadIndex = $process.Value.IndexOf('btstack_uart_h5_rx_dma_try_preload();')
$parseIndex = $process.Value.IndexOf('while (btstack_uart_h5_rx_have_frame_buffer)')
if (($reloadIndex -lt 0) -or ($parseIndex -lt 0) -or ($reloadIndex -gt $parseIndex)) {
    throw 'Run loop must preload UART RX DMA before parsing completed blocks'
}
if (($process.Value -notmatch 'btstack_uart_h5_rx_restart_pending\s*&&\s*!btstack_uart_h5_rx_dma_running') -or
    ($process.Value -notmatch 'btstack_uart_h5_rx_dma_release_channel')) {
    throw 'Stopped DMA recovery does not release the occupied channel in run-loop context'
}

$close = [regex]::Match(
    $uart,
    '(?s)static\s+int\s+btstack_uart_gpcv2178_close\s*\([^;]*?\)\s*\{.*?\n\}')
if (-not $close.Success -or $close.Value -notmatch 'btstack_uart_h5_rx_dma_release_channel') {
    throw 'Close path does not release the preloaded DMA channel'
}

Write-Host 'UART RX DMA refill is deferred from ISR to the BTstack run loop'
