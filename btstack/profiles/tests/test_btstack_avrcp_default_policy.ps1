$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$controllerPath = Join-Path $root 'btstack\profiles\btstack_avrcp_controller.c'
$managerPath = Join-Path $root 'btstack\core\btstack_profile_manager.c'
$productionPaths = @(
    'btstack\profiles\btstack_avrcp.c',
    'btstack\profiles\btstack_avrcp_controller.c',
    'btstack\profiles\btstack_avrcp_internal.h',
    'btstack\include\profiles\btstack_avrcp.h',
    'btstack\core\btstack_profile_manager.c',
    'btstack\core\btstack_core_dispatcher.c',
    'btstack\core\btstack_event_mapper.c',
    'btstack\core\btstack_core_internal.h',
    'btstack\include\core\btstack_profile_manager.h',
    'btstack\api\bt_xstack_api_core.c',
    'btstack\include\api\bt_xstack_audio.h',
    'btstack\include\api\bt_xstack_types.h',
    'btstack\include\core\bt_xstack_audio.h',
    'btstack\include\core\bt_xstack_types.h',
    'btstack\at\btstack_at.c',
    'btstack\at\btstack_at_command_table.c',
    'btstack\at\btstack_at_core_adapter.c',
    'btstack\at\btstack_at_dispatch_table.c',
    'btstack\at\btstack_at_internal.h',
    'btstack\at\btstack_at_parser.c'
)

$controller = Get-Content -Raw -LiteralPath $controllerPath
$manager = Get-Content -Raw -LiteralPath $managerPath

if ($controller -notmatch 'BTSTACK_AVRCP_PROGRESS_INTERVAL_MS\s*=\s*1000u') {
    throw 'AVRCP Controller must own a fixed 1000 ms progress interval'
}
if ($controller -notmatch '(?s)btstack_avrcp_progress_timeout.*?btstack_avrcp_play_status\s*!=\s*AVRCP_PLAYBACK_STATUS_PLAYING.*?BTSTACK_AVRCP_PENDING_PLAY_STATUS') {
    throw 'AVRCP progress timer must poll only while playback is active'
}
if ($controller -notmatch '(?s)AVRCP_SUBEVENT_NOTIFICATION_PLAYBACK_STATUS_CHANGED:.*?btstack_avrcp_update_progress_timer\s*\(\s*\)') {
    throw 'Playback notifications must update the fixed progress timer'
}
if ($controller -notmatch '(?s)AVRCP_SUBEVENT_PLAY_STATUS:.*?btstack_avrcp_update_progress_timer\s*\(\s*\)') {
    throw 'Play-status responses must update the fixed progress timer'
}
if ($manager -notmatch '(?s)BTSTACK_AVRCP_EVENT_CONNECTED.*?btstack_avrcp_request_metadata\s*\(\s*\)') {
    throw 'AVRCP connection must request metadata automatically'
}
if ($manager -notmatch '(?s)BTSTACK_AVRCP_EVENT_TRACK_CHANGED:.*?btstack_avrcp_request_metadata\s*\(\s*\)') {
    throw 'AVRCP track changes must request metadata automatically'
}

$forbidden = @(
    'btstack_avrcp_set_progress_interval',
    'btstack_avrcp_controller_set_progress_interval',
    'bt_xstack_get_avrcp_config',
    'bt_xstack_set_avrcp_config',
    'BT_XSTACK_REQUEST_SET_AVRCP_CONFIG',
    'BTSTACK_AT_COMMAND_AVRCPCFG',
    'AVRCPCFG'
)
foreach ($relativePath in $productionPaths) {
    $text = Get-Content -Raw -LiteralPath (Join-Path $root $relativePath)
    foreach ($token in $forbidden) {
        if ($text -match [regex]::Escape($token)) {
            throw "Obsolete configurable AVRCP policy remains in ${relativePath}: $token"
        }
    }
}

Write-Host 'AVRCP Controller owns fixed metadata and 1-second play-progress policy'
