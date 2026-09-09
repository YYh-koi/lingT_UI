$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$uartPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_uart_gpcv2178.c'

foreach ($path in @($configPath, $uartPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing UART0 RX health diagnostic artifact: $path"
    }
}

$config = Get-Content -Raw -LiteralPath $configPath
$uart = Get-Content -Raw -LiteralPath $uartPath

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_HEALTH_PERIOD_MS\s+1000u') {
    throw 'Missing one-second UART health period'
}
foreach ($token in @(
        'bytes=%u',
        'frames=%u',
        'dma_rem=%u',
        'owned=%u',
        'queued=%u',
        'dbf=%d',
        'preload=%u',
        'full_irq=%u',
        'idle_flush=%u',
        'uart=0x%08x',
        'rxerr=0x%08x')) {
    if ($uart -notmatch [regex]::Escape($token)) {
        throw "Missing UART health field: $token"
    }
}
if ($uart -notmatch 'btstack_uart_h5_rx_byte_count\s*\+=\s*BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE') {
    throw 'Full DMA blocks are not counted'
}
if ($uart -notmatch 'btstack_uart_h5_rx_byte_count\s*\+=\s*received') {
    throw 'Idle-flushed DMA bytes are not counted'
}
if ($uart -notmatch 'dma_get_tx_count\s*\(') {
    throw 'DMA remaining count is not sampled'
}
if (($uart -notmatch 'R_UART0_STATUS') -or
    ($uart -notmatch 'R_UART0_RX_STATUS')) {
    throw 'UART status registers are not sampled'
}
if ($uart -notmatch 'btstack_run_loop_set_timer_handler\s*\(\s*&btstack_uart_h5_rx_health_timer') {
    throw 'Health timer is not registered'
}
if ($uart -notmatch 'btstack_run_loop_remove_timer\s*\(\s*&btstack_uart_h5_rx_health_timer') {
    throw 'Health timer is not removed on close'
}
$handler = [regex]::Match(
    $uart,
    '(?s)static\s+void\s+btstack_uart_h5_rx_health_timer_handler\s*\([^)]*\)\s*\{.*?\n\}')
if (-not $handler.Success) {
    throw 'Missing UART health timer handler'
}
if ($handler.Value -match 'R_UART0_DATA') {
    throw 'UART health timer must not consume UART RX data'
}

Write-Host 'UART0 RX health diagnostics are configured'
