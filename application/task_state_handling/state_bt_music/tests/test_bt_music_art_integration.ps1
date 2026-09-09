$ErrorActionPreference = 'Stop'

$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $testDir '..\..\..\..')).Path

function Read-Text([string]$relativePath) {
    return Get-Content -Raw -LiteralPath (Join-Path $repoRoot $relativePath)
}

function Assert-Contains([string]$text, [string]$token, [string]$message) {
    if (-not $text.Contains($token)) {
        throw $message
    }
}

function Assert-NotContains([string]$text, [string]$token, [string]$message) {
    if ($text.Contains($token)) {
        throw $message
    }
}

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) {
        throw $message
    }
}

$systemMessages = Read-Text 'application\inc\turnkey_sys_msg.h'
$driverMessages = Read-Text 'application\inc\turnkey_drv_msg.h'
$stateTask = Read-Text 'application\task_state_handling\src\task_state_handling.c'
$browse = Read-Text 'application\task_state_handling\state_browse\src\state_browse.c'
$music = Read-Text 'application\task_state_handling\state_bt_music\src\bt_music_art.c'
$displayTask = Read-Text 'application\task_display\src\task_display.c'
$displayFunctions = Read-Text 'application\task_display\src\ap_display_func.c'
$mainSource = Read-Text 'Project\GPCV2178_Turnkey_SDK\src\main.c'
$customerConfig = Read-Text 'Project\GPCV2178_Turnkey_SDK\inc\customer.h'
$boardConfig = Read-Text 'board_config\inc\board_config.h'
$project = Read-Text 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$generatedMake = Read-Text 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release_Debug.mk'
$generatedMakeNormalized = $generatedMake.Replace('\', '/') -replace '/+', '/'
$coreCoverHeader = Read-Text 'btstack\include\core\bt_xstack_cover_art.h'
$apiCoverHeader = Read-Text 'btstack\include\api\bt_xstack_cover_art.h'
$avrcpController = Read-Text 'btstack\profiles\btstack_avrcp_controller.c'
$avrcpCoverArt = Read-Text 'btstack\profiles\btstack_avrcp_cover_art.c'
$bipClient = Read-Text 'btstack\profiles\btstack_bip_client.c'

Assert-Contains $systemMessages 'STATE_BT_MUSIC' 'STATE_BT_MUSIC is missing'
Assert-Contains $driverMessages 'MSG_APQ_BT_COVER_READY' 'cover-ready application message is missing'
Assert-Match $stateTask 'case\s+STATE_BT_MUSIC\s*:[\s\S]*?bt_music_art_state_entry' `
    'State Handling Task does not dispatch Bluetooth music'
Assert-NotContains $stateTask 'bt_music_art_route_media_keys' `
    'State Handling Task must not replace the global Bluetooth key routing'
Assert-NotContains $music 'bt_music_art_route_media_keys' `
    'Bluetooth music state must not replace the global Bluetooth key routing'
Assert-Match $browse 'case\s+MSG_APQ_MODE\s*:[\s\S]*?STATE_BT_MUSIC' `
    'Browse MODE does not enter Bluetooth music'
Assert-Match $music 'case\s+MSG_APQ_MODE\s*:[\s\S]*?STATE_VIDEO_RECORD' `
    'Bluetooth music MODE does not return to video record'
Assert-Match $music 'MSG_APQ_FUNCTION_KEY_ACTIVE[\s\S]*?bt_xstack_avrcp_play_pause' `
    'Bluetooth music OK does not control play/pause'
Assert-Match $music 'MSG_APQ_PREV_KEY_ACTIVE[\s\S]*?bt_xstack_avrcp_previous' `
    'Bluetooth music PREV does not select the previous track'
Assert-Match $music 'MSG_APQ_NEXT_KEY_ACTIVE[\s\S]*?bt_xstack_avrcp_next' `
    'Bluetooth music NEXT does not select the next track'
Assert-Contains $project '../../application/task_state_handling/state_bt_music/src/bt_music_art.c' `
    'Main project does not compile bt_music_art.c'
Assert-Contains $project '../../application/task_state_handling/state_bt_music/inc' `
    'Main project does not include the Bluetooth music header directory'
Assert-Contains $generatedMakeNormalized 'state_bt_music/src/bt_music_art.c' `
    'Generated main makefile does not compile bt_music_art.c'
Assert-Contains $coreCoverHeader 'bt_xstack_cover_art_register_callback' `
    'Core Cover Art header is missing runtime callback registration'
Assert-Contains $apiCoverHeader 'bt_xstack_cover_art_register_callback' `
    'API Cover Art header is missing runtime callback registration'
Assert-Match $displayTask `
    'msg_id\s*==\s*MSG_DISPLAY_TASK_JPEG_DRAW\s*\|\|\s*Get_1st_decode_frm\s*==\s*0' `
    'Static JPEG frames must not be blocked by the video first-frame gate'
Assert-Match $customerConfig '#define\s+C_TFT_DISPLAY_FUNC\s+CUSTOM_ON' `
    'The CN1 TFT output must be enabled for Bluetooth Cover Art display'
Assert-Match $customerConfig '#define\s+LCD_DRV_EN_T43\s+0' `
    'The T43 timing must stay disabled for the AUO panel'
Assert-Match $customerConfig '#define\s+LCD_DRV_EN_AUO_A027DTN019\s+1' `
    'The working AUO A027DTN019 panel timing must be selected'
Assert-Match $boardConfig '#define\s+TFT1_ENABLE\s+1' `
    'Bluetooth Cover Art must use the TFT1 output connected to the panel'
Assert-Match $boardConfig '#define\s+TFT2_ENABLE\s+0' `
    'TFT2 must be disabled when the panel is connected to TFT1'
Assert-Match $displayFunctions `
    'drv_l2_display_start\s*\([^;]+;[\s\S]*?drv_l2_tft_backlight_en_set\s*\(\s*TRUE\s*\)' `
    'Display startup must physically enable the panel backlight after starting TFT output'
Assert-Contains $music 'bt_music_art_present_test_pattern' `
    'Bluetooth music entry must submit the RGB display-path test pattern'
Assert-NotContains $displayFunctions 'display init: backlight=on' `
    'Display startup hardware-diagnosis logging must be removed'
Assert-NotContains $displayTask 'display task: jpeg received' `
    'Display receive-path diagnosis logging must be removed'
Assert-NotContains $displayFunctions 'display task: jpeg queued' `
    'Display ready-queue diagnosis logging must be removed'
Assert-NotContains $mainSource 'TFT_DIAG(' `
    'TFT register diagnosis logging must be removed'
Assert-NotContains $displayFunctions 'tft_last_' `
    'TFT register diagnosis state must be removed from the VBlank path'
Assert-NotContains $mainSource 'tft_last_' `
    'TFT register diagnosis globals must be removed'
Assert-NotContains $music 'DBG_PRINT("bt music art:' `
    'Bluetooth music Cover Art diagnosis logging must be removed'
Assert-NotContains $avrcpController 'BTSTACK_PRINTF' `
    'AVRCP metadata diagnosis logging must be removed'
Assert-NotContains $avrcpCoverArt 'BTSTACK_PRINTF' `
    'AVRCP Cover Art diagnosis logging must be removed'
Assert-NotContains $bipClient 'BTSTACK_PRINTF' `
    'BIP client diagnosis logging must be removed'

Write-Output 'PASS: Bluetooth music state and key integration'
