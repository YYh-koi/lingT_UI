$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$includeRoot = Join-Path $apiRoot 'include\api'

function Read-RequiredFile([string]$name) {
    $path = Join-Path $includeRoot $name
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing public API header: $name"
    }
    return Get-Content -Raw -LiteralPath $path
}

function Assert-Match(
    [string]$text,
    [string]$pattern,
    [string]$message)
{
    if ($text -notmatch $pattern) { throw $message }
}

$umbrella = Read-RequiredFile 'bt_xstack.h'
$types = Read-RequiredFile 'bt_xstack_types.h'
$service = Read-RequiredFile 'bt_xstack_service.h'
$phone = Read-RequiredFile 'bt_xstack_phone.h'
$audio = Read-RequiredFile 'bt_xstack_audio.h'
$phonebook = Read-RequiredFile 'bt_xstack_phonebook.h'
$all = $umbrella + $types + $service + $phone + $audio + $phonebook

foreach ($header in @(
        'bt_xstack_types.h',
        'bt_xstack_service.h',
        'bt_xstack_phone.h',
        'bt_xstack_audio.h',
        'bt_xstack_phonebook.h')) {
    Assert-Match $umbrella ([regex]::Escape("#include `"$header`"")) `
        "Umbrella header does not include $header"
}

foreach ($forbidden in @(
        '\bbt_xstack_send_at\s*\(',
        '\bbtstack_at_',
        '#include\s+["<]at/')) {
    if ($all -match $forbidden) {
        throw "Public API still exposes AT dependency: $forbidden"
    }
}

foreach ($type in @(
        'bt_xstack_result_t',
        'bt_xstack_addr_t',
        'bt_xstack_profile_t',
        'bt_xstack_state_t',
        'bt_xstack_service_event_t',
        'bt_xstack_connection_event_t',
        'bt_xstack_phone_event_t',
        'bt_xstack_audio_event_t',
        'bt_xstack_phonebook_event_t',
        'bt_xstack_config_t')) {
    Assert-Match $all ("\b" + [regex]::Escape($type) + "\b") `
        "Missing public type: $type"
}

foreach ($category in @(
        'BT_XSTACK_PHONEBOOK_CONTACTS',
        'BT_XSTACK_PHONEBOOK_INCOMING',
        'BT_XSTACK_PHONEBOOK_OUTGOING',
        'BT_XSTACK_PHONEBOOK_MISSED',
        'BT_XSTACK_PHONEBOOK_ALL')) {
    Assert-Match $all ("\b" + [regex]::Escape($category) + "\b") `
        "Missing public phonebook category selector: $category"
}

if ($all -match '\bBT_XSTACK_PHONEBOOK_CATEGORY_MASK_(CONTACTS|INCOMING|OUTGOING|MISSED|ALL)\b') {
    throw 'Legacy public phonebook CATEGORY_MASK names remain'
}

$functions = @(
    'bt_xstack_init',
    'bt_xstack_is_ready',
    'bt_xstack_enable',
    'bt_xstack_disable',
    'bt_xstack_get_state',
    'bt_xstack_get_version',
    'bt_xstack_get_profile_mask',
    'bt_xstack_set_profile_mask',
    'bt_xstack_get_local_address',
    'bt_xstack_get_local_name',
    'bt_xstack_set_local_name',
    'bt_xstack_get_pin',
    'bt_xstack_set_pin',
    'bt_xstack_get_ssp_mode',
    'bt_xstack_set_ssp_mode',
    'bt_xstack_pairing_confirm',
    'bt_xstack_set_discoverable',
    'bt_xstack_set_connectable',
    'bt_xstack_get_class_of_device',
    'bt_xstack_set_class_of_device',
    'bt_xstack_get_paired_device_count',
    'bt_xstack_get_paired_device',
    'bt_xstack_clear_paired_device_by_index',
    'bt_xstack_clear_paired_device_by_address',
    'bt_xstack_clear_paired_devices',
    'bt_xstack_get_tdl_mode',
    'bt_xstack_set_tdl_mode',
    'bt_xstack_scan_start',
    'bt_xstack_scan_stop',
    'bt_xstack_disconnect_all',
    'bt_xstack_connect',
    'bt_xstack_connect_last',
    'bt_xstack_disconnect',
    'bt_xstack_get_connection_state',
    'bt_xstack_get_hfp_config',
    'bt_xstack_set_hfp_config',
    'bt_xstack_get_hfp_state',
    'bt_xstack_hfp_dial',
    'bt_xstack_hfp_redial',
    'bt_xstack_hfp_send_dtmf',
    'bt_xstack_hfp_answer',
    'bt_xstack_hfp_hang_up',
    'bt_xstack_hfp_multiparty_control',
    'bt_xstack_hfp_toggle_audio',
    'bt_xstack_hfp_set_audio_connected',
    'bt_xstack_hfp_set_voice_recognition',
    'bt_xstack_get_microphone_mute',
    'bt_xstack_set_microphone_mute',
    'bt_xstack_get_speaker_volume',
    'bt_xstack_set_speaker_volume',
    'bt_xstack_get_a2dp_state',
    'bt_xstack_get_a2dp_mute',
    'bt_xstack_set_a2dp_mute',
    'bt_xstack_get_avrcp_state',
    'bt_xstack_get_avrcp_play_state',
    'bt_xstack_avrcp_play_pause',
    'bt_xstack_avrcp_play',
    'bt_xstack_avrcp_pause',
    'bt_xstack_avrcp_stop',
    'bt_xstack_avrcp_next',
    'bt_xstack_avrcp_previous',
    'bt_xstack_avrcp_fast_forward',
    'bt_xstack_avrcp_rewind',
    'bt_xstack_avrcp_set_repeat',
    'bt_xstack_avrcp_set_shuffle',
    'bt_xstack_avrcp_volume_up',
    'bt_xstack_avrcp_volume_down',
    'bt_xstack_call_audio_write',
    'bt_xstack_phonebook_sync',
    'bt_xstack_phonebook_cancel',
    'bt_xstack_phonebook_authenticate',
    'bt_xstack_phonebook_get_state'
)
foreach ($function in $functions) {
    Assert-Match $all ("\b" + [regex]::Escape($function) + "\s*\(") `
        "Missing public API function: $function"
}

foreach ($legacy in @(
        'bt_xstack_pbap_sync',
        'bt_xstack_pbap_cancel',
        'bt_xstack_pbap_authenticate',
        'bt_xstack_get_pbap_state')) {
    if ($all -match ("\b" + [regex]::Escape($legacy) + "\s*\(")) {
        throw "Legacy public PBAP API remains: $legacy"
    }
}

Write-Output 'xStack typed public API covers the current AT command surface'
