$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourcePath = Join-Path $repoRoot 'application\task_peripheral_handling\src\ap_peripheral_handling.c'
$source = Get-Content -LiteralPath $sourcePath -Raw

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

Assert-NotMatch $mode 'ap_peripheral_(media|call)_key_dispatch' `
    'MODE must be reserved for application page switching'
Assert-Match $mode 'app_msg_send\s*\(\s*MSG_APQ_MODE\b' `
    'MODE must send MSG_APQ_MODE'

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

Write-Host 'Bluetooth key routing contract passed.'
