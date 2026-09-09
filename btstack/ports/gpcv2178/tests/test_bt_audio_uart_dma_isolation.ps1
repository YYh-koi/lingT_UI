$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$uartPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_uart_gpcv2178.c'
$audioPath = Join-Path $root 'Project/wifi_demo/bt_audio_task.c'
$audioConfigPath = Join-Path $root 'Project/Include/bt_audio_task.h'

foreach ($path in @($configPath, $uartPath, $audioPath, $audioConfigPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing A2DP/UART DMA isolation artifact: $path"
    }
}

$config = Get-Content -Raw -LiteralPath $configPath
$uart = Get-Content -Raw -LiteralPath $uartPath
$audio = Get-Content -Raw -LiteralPath $audioPath
$audioConfig = Get-Content -Raw -LiteralPath $audioConfigPath

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_DIAG_VERBOSE\s+0') {
    throw 'UART RX DMA positioning logs are not disabled for normal playback'
}
$dacBypassEnabled = $audioConfig -match '#define\s+BT_AUDIO_DIAG_BYPASS_DAC_DMA\s+1'
$dacPlaybackEnabled = $audioConfig -match '#define\s+BT_AUDIO_DIAG_BYPASS_DAC_DMA\s+0'
if (-not ($dacBypassEnabled -or $dacPlaybackEnabled)) {
    throw 'DAC DMA diagnostic mode is not configured'
}
foreach ($token in @(
        'dma_ch=%u',
        'running=%u',
        'ready=%u',
        'drop=%u',
        'error=%u')) {
    if ($uart -notmatch [regex]::Escape($token)) {
        throw "UART RX DMA diagnostic is missing: $token"
    }
}
if ($dacBypassEnabled -and
    ($audio -notmatch '(?s)#if\s+BT_AUDIO_DIAG_BYPASS_DAC_DMA.*?bt audio diag: DAC DMA bypassed.*?return\s+0\s*;.*?#endif')) {
    throw 'Audio demo does not bypass DAC DMA while retaining the PCM path'
}
if ($dacPlaybackEnabled -and
    ($audio -notmatch '(?s)#else.*?drv_l1_dac_cha_dbf_put_callback.*?drv_l1_dac_cha_dbf_set.*?ap_peripheral_speaker_ctrl\s*\(\s*1\s*\).*?#endif')) {
    throw 'Audio demo does not start DAC DMA and speaker output in playback mode'
}
if ($audio -notmatch '(?s)static\s+void\s+bt_audio_task_stop_dac\s*\([^)]*\).*?#if\s+BT_AUDIO_DIAG_BYPASS_DAC_DMA.*?return\s*;.*?#else.*?drv_l1_dac_dbf_free\s*\(\s*\).*?#endif') {
    throw 'DAC stop path is unsafe when no diagnostic DAC DMA was allocated'
}
if ($audio -notmatch '(?s)#if\s*!BT_AUDIO_DIAG_BYPASS_DAC_DMA.*?static\s+void\s+bt_audio_dac_isr.*?#endif') {
    throw 'Unused DAC ISR is still compiled into the DAC-bypass diagnostic build'
}
if ($config -notmatch '#define\s+BTSTACK_GPCV2178_HCI_DUMP_ENABLE\s+0') {
    throw 'Full HCI dump must remain disabled during the isolation test'
}

Write-Host 'A2DP DAC mode is configured and UART RX DMA positioning logs are disabled'
