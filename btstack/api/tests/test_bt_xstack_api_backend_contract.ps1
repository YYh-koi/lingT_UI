$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent $PSScriptRoot
$backendPath = Join-Path $apiRoot '..\core\btstack_core_dispatcher.c'
$eventPath = Join-Path $apiRoot '..\core\btstack_event_mapper.c'
$managerPath = Join-Path $apiRoot '..\core\btstack_profile_manager.c'
if (-not (Test-Path -LiteralPath $backendPath)) {
    throw 'Missing typed API backend'
}
if (-not (Test-Path -LiteralPath $eventPath)) {
    throw 'Missing typed API event mapper'
}
if (-not (Test-Path -LiteralPath $managerPath)) {
    throw 'Missing Profile Manager'
}
$backend = Get-Content -Raw -LiteralPath $backendPath
$events = Get-Content -Raw -LiteralPath $eventPath
$manager = Get-Content -Raw -LiteralPath $managerPath
$all = $backend + $events + $manager

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) { throw $message }
}
function Assert-NoMatch([string]$text, [string]$pattern, [string]$message) {
    if ($text -match $pattern) { throw $message }
}

foreach ($request in @(
        'SET_POWER','SET_PROFILE_MASK','SET_NAME','SET_PIN','SET_SSP',
        'PAIR_CONFIRM','SET_DISCOVERABLE','SET_CONNECTABLE','SET_COD',
        'CLEAR_PEER_INDEX','CLEAR_PEER_ADDRESS','CLEAR_PEERS','SET_TDL',
        'SCAN_START','SCAN_STOP','DISCONNECT_ALL','CONNECT','CONNECT_LAST',
        'DISCONNECT_PROFILE','SET_HFP_CONFIG','HFP_DIAL','HFP_REDIAL',
        'HFP_DTMF','HFP_CONTROL','SET_MICROPHONE_MUTE',
        'SET_SPEAKER_VOLUME','SET_A2DP_MUTE',
        'AVRCP_CONTROL','SET_REPEAT','SET_SHUFFLE','PBAP_SYNC',
        'PBAP_CANCEL','PBAP_AUTHENTICATE','PBAP_EVENT_SPACE_AVAILABLE',
        'PBAP_PEER_DISCONNECTED')) {
    Assert-Match $backend ("BT_XSTACK_REQUEST_" + $request) `
        "Missing backend request case: $request"
}

foreach ($native in @(
        'btstack_adapter_set_power','btstack_adapter_set_name',
        'btstack_adapter_set_pin','btstack_adapter_set_ssp_mode',
        'btstack_adapter_pairing_confirm','btstack_adapter_scan_start',
        'btstack_adapter_scan_stop','btstack_profile_manager_connect',
        'btstack_profile_manager_disconnect',
        'btstack_profile_manager_disconnect_all',
        'btstack_hfp_hf_execute_control','btstack_avrcp_execute_control',
        'btstack_pbap_client_sync','btstack_pbap_client_cancel',
        'btstack_pbap_client_authenticate',
        'btstack_pbap_client_peer_disconnected',
        'btstack_platform_post_stack_callback')) {
    Assert-Match $backend ("\b" + $native + "\s*\(") `
        "Missing native backend call: $native"
}

foreach ($callback in @(
        'btstack_adapter_set_event_callback',
        'btstack_hfp_hf_set_direct_audio_callback')) {
    Assert-Match $all ("\b" + $callback + "\s*\(") `
        "Missing native event callback registration: $callback"
}
Assert-Match $all '\bbtstack_pbap_client_set_event_callback\s*\(' `
    'Missing generic PBAP event callback registration'
foreach ($profileCallback in @(
        'hfp_config.event_callback', 'a2dp_config.event_callback',
        'avrcp_config.event_callback', 'audio_sample_callback')) {
    Assert-Match $manager ([regex]::Escape($profileCallback)) `
        "Profile Manager does not own callback binding: $profileCallback"
}
Assert-NoMatch $events '\bbtstack_(hfp_hf_set_event_callback|pbap_client_set_event_callback|a2dp_sink_set_event_callback|avrcp_set_event_callback)\s*\(' `
    'Event Mapper must not own Classic profile subscriptions'
Assert-NoMatch $all '#include\s+"gap/' `
    'Core routing and event mapping must use the Adapter GAP facade'

foreach ($domain in @(
        'BT_XSTACK_EVENT_DOMAIN_SERVICE',
        'BT_XSTACK_EVENT_DOMAIN_CONNECTION',
        'BT_XSTACK_EVENT_DOMAIN_PHONE',
        'BT_XSTACK_EVENT_DOMAIN_AUDIO',
        'BT_XSTACK_EVENT_DOMAIN_PHONEBOOK')) {
    Assert-Match $events $domain "Missing typed event domain: $domain"
}

Assert-Match $backend 'btstack_core_command_slots\s*\[' `
    'Backend must own fixed prepared-command slots'
Assert-NoMatch $backend 'btstack_core_complete\s*\(' `
    'Backend must not synthesize an internal command-completion handshake'
Assert-NoMatch $all '\bbtstack_at_' `
    'Typed API backend must not depend on AT code'
Assert-NoMatch $all '\bmalloc\s*\(' `
    'Typed API backend must not allocate from the heap'

Write-Output 'xStack typed backend covers all current command and event domains'
