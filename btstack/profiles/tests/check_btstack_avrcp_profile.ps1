$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))

function Read-RequiredFile([string]$relativePath) {
    $path = Join-Path $root $relativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing AVRCP split file: $relativePath"
    }
    return Get-Content -Raw -LiteralPath $path
}

function Assert-Contains([string]$text, [string]$token, [string]$message) {
    if ($text -notmatch [regex]::Escape($token)) {
        throw $message
    }
}

function Assert-NotContains([string]$text, [string]$token, [string]$message) {
    if ($text -match [regex]::Escape($token)) {
        throw $message
    }
}

$common = Read-RequiredFile 'btstack/profiles/btstack_avrcp.c'
$controller = Read-RequiredFile 'btstack/profiles/btstack_avrcp_controller.c'
$target = Read-RequiredFile 'btstack/profiles/btstack_avrcp_target.c'
$internal = Read-RequiredFile 'btstack/profiles/btstack_avrcp_internal.h'

@(
    'avrcp_init',
    'avrcp_register_packet_handler',
    'avrcp_connect',
    'avrcp_disconnect',
    'AVRCP_SUBEVENT_CONNECTION_ESTABLISHED',
    'AVRCP_SUBEVENT_CONNECTION_RELEASED',
    'btstack_avrcp_controller_setup',
    'btstack_avrcp_target_setup',
    'btstack_avrcp_controller_connected',
    'btstack_avrcp_target_connected'
) | ForEach-Object {
    Assert-Contains $common $_ "Common AVRCP session layer is missing $_"
}

Assert-NotContains $common 'classic/avrcp_controller.h' `
    'Common AVRCP session layer must not include controller role APIs'
Assert-NotContains $common 'classic/avrcp_target.h' `
    'Common AVRCP session layer must not include target role APIs'
Assert-NotContains $common 'avrcp_controller_register_packet_handler' `
    'Controller packet ownership must stay in the CT module'
Assert-NotContains $common 'avrcp_target_register_packet_handler' `
    'Target packet ownership must stay in the TG module'

@(
    'classic/avrcp_controller.h',
    'avrcp_controller_init',
    'avrcp_controller_register_packet_handler',
    'avrcp_controller_create_sdp_record',
    'AVRCP_SUBEVENT_NOTIFICATION_PLAYBACK_STATUS_CHANGED',
    'AVRCP_SUBEVENT_NOW_PLAYING_INFO_DONE',
    'avrcp_controller_play',
    'avrcp_controller_get_now_playing_info'
) | ForEach-Object {
    Assert-Contains $controller $_ "AVRCP CT module is missing $_"
}
Assert-NotContains $controller 'classic/avrcp_target.h' `
    'AVRCP CT module must not depend on target role APIs'
Assert-NotContains $controller 'avrcp_target_' `
    'AVRCP CT module must not invoke target role APIs'

@(
    'classic/avrcp_target.h',
    'avrcp_target_init',
    'avrcp_target_register_packet_handler',
    'avrcp_target_create_sdp_record',
    'AVRCP_SUBEVENT_PLAY_STATUS_QUERY',
    'avrcp_target_play_status',
    'avrcp_target_support_event'
) | ForEach-Object {
    Assert-Contains $target $_ "AVRCP TG module is missing $_"
}
Assert-NotContains $target 'classic/avrcp_controller.h' `
    'AVRCP TG module must not depend on controller role APIs'
Assert-NotContains $target 'avrcp_controller_' `
    'AVRCP TG module must not invoke controller role APIs'

@(
    'btstack_avrcp_internal_get_cid',
    'btstack_avrcp_internal_emit',
    'btstack_avrcp_controller_setup',
    'btstack_avrcp_controller_connected',
    'btstack_avrcp_controller_disconnected',
    'btstack_avrcp_target_setup',
    'btstack_avrcp_target_connected',
    'btstack_avrcp_target_disconnected'
) | ForEach-Object {
    Assert-Contains $internal $_ "AVRCP private boundary is missing $_"
}

Write-Host 'BTstack AVRCP common/CT/TG split contract passed'
