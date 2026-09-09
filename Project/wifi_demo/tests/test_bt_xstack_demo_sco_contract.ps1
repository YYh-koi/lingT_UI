$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $demoRoot 'bt_xstack_demo.c'
$source = Get-Content -Raw -LiteralPath $sourcePath
$phoneMatch = [regex]::Match(
    $source,
    '(?s)static\s+void\s+bt_xstack_demo_phone_event\s*\(.*?(?=static\s+void\s+bt_xstack_demo_connection_event)')
if (-not $phoneMatch.Success) {
    throw 'Cannot locate typed phone callback'
}
$phoneCallback = $phoneMatch.Value

if ($phoneCallback -notmatch 'BT_XSTACK_PHONE_EVENT_AUDIO') {
    throw 'Demo does not handle typed HFP AUDIO events'
}
if ($phoneCallback -notmatch '(?s)BT_XSTACK_PHONE_EVENT_AUDIO.{0,1200}event->values\[0\].{0,1200}bt_audio_task_configure\s*\(\s*8000u\s*,\s*1u\s*,\s*16u\s*\)') {
    throw 'SCO connect does not configure 8 kHz mono PCM'
}
if ($phoneCallback -notmatch '(?s)BT_XSTACK_PHONE_EVENT_AUDIO.{0,1800}bt_audio_task_stop\s*\(') {
    throw 'SCO release does not stop playback'
}
$stateBranch = [regex]::Match(
    $phoneCallback,
    '(?s)\(event->id\s*==\s*BT_XSTACK_PHONE_EVENT_STATE\).*?(?=\}\s*else\s+if\s*\(\(event->id\s*==\s*BT_XSTACK_PHONE_EVENT_AUDIO)')
if (-not $stateBranch.Success) {
    throw 'Cannot separate call-state and SCO-audio lifecycle branches'
}
if ($stateBranch.Value -match 'bt_audio_task_configure') {
    throw 'Call state configures DAC before SCO is connected'
}

Write-Output 'Typed HFP AUDIO event controls SCO playback lifecycle'
