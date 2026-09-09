$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$audioPath = Join-Path $root 'Project/wifi_demo/bt_audio_task.c'
$audioConfigPath = Join-Path $root 'Project/Include/bt_audio_task.h'
$peripheralPath = Join-Path $root 'application/task_peripheral_handling/src/ap_peripheral_handling.c'

foreach ($path in @($configPath, $audioPath, $audioConfigPath, $peripheralPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing Bluetooth DAC playback artifact: $path"
    }
}

$config = Get-Content -Raw -LiteralPath $configPath
$audio = Get-Content -Raw -LiteralPath $audioPath
$audioConfig = Get-Content -Raw -LiteralPath $audioConfigPath
$peripheral = Get-Content -Raw -LiteralPath $peripheralPath

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE\s+512u') {
    throw 'UART RX DMA chunk must remain at the verified 512-byte size'
}
if ($audioConfig -notmatch '#define\s+BT_AUDIO_DIAG_BYPASS_DAC_DMA\s+0') {
    throw 'DAC DMA playback is still bypassed'
}
if ($audio -notmatch '(?s)drv_l1_dac_cha_dbf_put_callback\s*\(.*?drv_l1_dac_cha_dbf_set\s*\(.*?ap_peripheral_speaker_ctrl\s*\(\s*1\s*\)') {
    throw 'DAC double-buffer startup does not enable the speaker output'
}
if ($audio -notmatch '(?s)bt_audio_task_stop_dac\s*\([^)]*\).*?ap_peripheral_speaker_ctrl\s*\(\s*0\s*\).*?drv_l1_dac_timer_stop\s*\(\s*\).*?drv_l1_dac_dbf_free\s*\(\s*\)') {
    throw 'DAC stop path does not disable the speaker and release DMA'
}
if ($peripheral -notmatch '(?s)ap_peripheral_speaker_ctrl\s*\([^)]*\)\s*\{\s*if\s*\(\s*on_or_off\s*\).*?drv_l1_dac_audop_speaker_en\s*\(\s*1\s*\).*?#ifdef\s+SPEAKER_EN_PIN') {
    throw 'Internal DAC speaker output is still incorrectly gated by external SPEAKER_EN_PIN'
}

Write-Host 'Bluetooth DAC DMA playback and internal speaker output are enabled'
