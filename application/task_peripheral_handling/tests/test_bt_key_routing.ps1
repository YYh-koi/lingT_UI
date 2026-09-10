$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourcePath = Join-Path $repoRoot 'application\task_peripheral_handling\src\ap_peripheral_handling.c'
$taskSourcePath = Join-Path $repoRoot 'application\task_peripheral_handling\src\task_peripheral_handling.c'
$source = Get-Content -LiteralPath $sourcePath -Raw
$taskSource = Get-Content -LiteralPath $taskSourcePath -Raw

function Get-FunctionBody([string]$name) {
    $match = [regex]::Match($source, "void\s+$name\s*\([^)]*\)\s*\{")
    if (-not $match.Success) {
        throw "Missing function: $name"
    }

    $openBrace = $match.Index + $match.Length - 1
    $depth = 0
    for ($index = $openBrace; $index -lt $source.Length; ++$index) {
        if ($source[$index] -eq '{') {
            ++$depth
        } elseif ($source[$index] -eq '}') {
            --$depth
            if ($depth -eq 0) {
                return $source.Substring($openBrace + 1, $index - $openBrace - 1)
            }
        }
    }

    throw "Unterminated function: $name"
}

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) {
        throw $message
    }
}

function Assert-NotMatch([string]$text, [string]$pattern, [string]$message) {
    if ($text -match $pattern) {
        throw $message
    }
}

function Assert-HandledDispatch(
    [string]$body,
    [string]$dispatcher,
    [string]$action,
    [string]$keyName) {
    $pattern = "if\s*\(\s*$dispatcher\s*\(\s*$action\s*\)\s*\)\s*\{\s*return\s*;\s*\}"
    Assert-Match $body $pattern "$keyName must consume $action when the Bluetooth handler accepts it"
}

$mode = Get-FunctionBody 'ap_peripheral_mode_key_exe'
$ok = Get-FunctionBody 'ap_peripheral_ok_key_exe'
$previous = Get-FunctionBody 'ap_peripheral_prev_key_exe'
$next = Get-FunctionBody 'ap_peripheral_next_key_exe'
$menu = Get-FunctionBody 'ap_peripheral_menu_key_exe'
$sos = Get-FunctionBody 'ap_peripheral_sos_key_exe'

Assert-Match $source 'ADKEY1: previous track\s*\*/\s*ap_peripheral_prev_key_exe\s*,\s*ADKEY_OK_L' `
    'ADKEY1 must route to previous track'
Assert-Match $source 'ADKEY2: play/pause\s*\*/\s*ap_peripheral_ok_key_exe\s*,\s*ADKEY_PREV_L' `
    'ADKEY2 must route to play/pause'
Assert-Match $source 'ap_peripheral_next_key_exe\s*,\s*ADKEY_NEXT_L' `
    'ADKEY3 must route to next track'
Assert-Match $source 'ADKEY1\.\.6 use the board[\s\S]*?#if ADKEY_NUM\s*//ad_value_cnt = 0;\s*drv_l1_adc_init' `
    'ADKEY ADC acquisition must remain enabled for LVGL'
Assert-Match $taskSource '#if ADKEY_NUM\s*/\* The six resistor keys[\s\S]*?ap_peripheral_ad_key_judge\(\);' `
    'Peripheral polling must scan ADKEYs when LVGL is enabled'
Assert-Match $source 'Dispatch once after two[\s\S]*?key_pressed_cnt >= C_KEY_STABLE_THRESHOLD[\s\S]*?key_function\(' `
    'ADKEY actions must dispatch while the key is pressed'
Assert-Match $source '!fast_key_exec_flag && !normal_key_exec_flag[\s\S]*?!long_key_exec_flag && key_pressed_cnt >= C_KEY_STABLE_THRESHOLD' `
    'Key release must not dispatch an action already sent on press'

Assert-HandledDispatch $mode 'ap_peripheral_media_key_dispatch' `
    'AP_PERIPHERAL_MEDIA_KEY_NEXT' 'MODE/KEY3'
Assert-Match $mode 'app_msg_send\s*\(\s*MSG_APQ_MODE\b' `
    'MODE must preserve page switching when Bluetooth does not claim KEY3'

Assert-HandledDispatch $ok 'ap_peripheral_media_key_dispatch' `
    'AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE' 'OK'
Assert-NotMatch $ok 'ap_peripheral_call_key_dispatch' `
    'OK must not answer calls'

Assert-HandledDispatch $previous 'ap_peripheral_media_key_dispatch' `
    'AP_PERIPHERAL_MEDIA_KEY_PREVIOUS' 'PREV'
Assert-NotMatch $previous 'ap_peripheral_call_key_dispatch' `
    'PREV must not hang up calls'

Assert-HandledDispatch $next 'ap_peripheral_media_key_dispatch' `
    'AP_PERIPHERAL_MEDIA_KEY_NEXT' 'NEXT'

Assert-HandledDispatch $menu 'ap_peripheral_call_key_dispatch' `
    'AP_PERIPHERAL_CALL_KEY_ANSWER' 'MENU'
Assert-NotMatch $menu 'ap_peripheral_media_key_dispatch' `
    'MENU must not control AVRCP'
Assert-Match $menu 'app_msg_send\s*\(\s*MSG_APQ_MENU_KEY_ACTIVE\b' `
    'MENU must preserve its normal fallback action'

Assert-HandledDispatch $sos 'ap_peripheral_call_key_dispatch' `
    'AP_PERIPHERAL_CALL_KEY_HANGUP' 'SOS'
Assert-Match $sos 'app_msg_send\s*\(\s*MSG_APQ_SOS_KEY_ACTIVE\b' `
    'SOS must preserve its normal fallback action'

Assert-Match $menu 'lvgl_page_show_dashboard\s*\(\s*\)' `
    'ADKEY5 must return from the music page to the dashboard'
Assert-Match $sos 'lvgl_page_show_music\s*\(\s*\)' `
    'ADKEY6 must open the music page from the dashboard'

Write-Host 'Bluetooth key routing contract passed.'
