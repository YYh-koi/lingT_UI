$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$api = Join-Path $root 'api'
$include = Join-Path $root 'include\api'
$headerPath = Join-Path $include 'bt_xstack_phonebook.h'

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}
function Forbid([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) { throw $Message }
}

if (-not (Test-Path -LiteralPath $headerPath)) {
    throw 'Missing public PBAP phonebook header'
}

$header = Get-Content -Raw -LiteralPath $headerPath
$umbrella = Get-Content -Raw -LiteralPath (Join-Path $include 'bt_xstack.h')
$internal = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_core_internal.h')
$core = Get-Content -Raw -LiteralPath (Join-Path $api 'bt_xstack_api_core.c')
$backend = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_core_dispatcher.c')
$events = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_event_mapper.c')
$manager = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_profile_manager.c')
$profile = Get-Content -Raw -LiteralPath (Join-Path $root 'profiles\btstack_pbap_client.c')

Require $umbrella '#include\s+"bt_xstack_phonebook\.h"' `
    'Umbrella header must include the public phonebook API'
Require $umbrella 'bt_xstack_phonebook_callback_t\s+phonebook_callback\s*;' `
    'Configuration must expose the phonebook callback'
Require $core 'config->phonebook_callback\s*==\s*bt_xstack_api_config\.phonebook_callback' `
    'Repeated initialization must compare the phonebook callback'

foreach ($constant in @(
        'BT_XSTACK_PHONEBOOK_NAME_MAX\s+48u',
        'BT_XSTACK_PHONEBOOK_NUMBER_MAX\s+32u',
        'BT_XSTACK_PHONEBOOK_NUMBER_COUNT\s+4u',
        'BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX\s+24u',
        'BT_XSTACK_PHONEBOOK_PASSWORD_MAX\s+32u')) {
    Require $header $constant "Missing public PBAP capacity: $constant"
}
foreach ($type in @(
        'bt_xstack_phonebook_state_t',
        'bt_xstack_phonebook_category_mask_t',
        'bt_xstack_phonebook_category_t',
        'bt_xstack_phonebook_status_t', 'bt_xstack_phonebook_direction_t',
        'bt_xstack_phonebook_number_t', 'bt_xstack_phonebook_contact_t',
        'bt_xstack_phonebook_call_t', 'bt_xstack_phonebook_event_t',
        'bt_xstack_phonebook_callback_t')) {
    Require $header ("\b" + $type + "\b") "Missing public PBAP type: $type"
}
foreach ($state in @(
        'IDLE', 'CONNECTING', 'AUTHENTICATING', 'PULLING', 'PAUSED',
        'DISCONNECTING')) {
    Require $header ("BT_XSTACK_PHONEBOOK_STATE_" + $state) `
        "Missing public phonebook state: $state"
}
foreach ($categoryMask in @(
        'CONTACTS', 'INCOMING', 'OUTGOING', 'MISSED', 'ALL')) {
    Require $header ("BT_XSTACK_PHONEBOOK_" + $categoryMask) `
        "Missing public phonebook category mask: $categoryMask"
}
Require $header 'BT_XSTACK_PHONEBOOK_CONTACTS\s*=\s*\(?\s*1u?\s*<<\s*0\s*\)?' `
    'Contacts category mask must be bit 0'
Require $header 'BT_XSTACK_PHONEBOOK_INCOMING\s*=\s*\(?\s*1u?\s*<<\s*1\s*\)?' `
    'Incoming category mask must be bit 1'
Require $header 'BT_XSTACK_PHONEBOOK_OUTGOING\s*=\s*\(?\s*1u?\s*<<\s*2\s*\)?' `
    'Outgoing category mask must be bit 2'
Require $header 'BT_XSTACK_PHONEBOOK_MISSED\s*=\s*\(?\s*1u?\s*<<\s*3\s*\)?' `
    'Missed category mask must be bit 3'
Require $header '(?s)BT_XSTACK_PHONEBOOK_ALL\s*=.*BT_XSTACK_PHONEBOOK_CONTACTS.*\|.*BT_XSTACK_PHONEBOOK_INCOMING.*\|.*BT_XSTACK_PHONEBOOK_OUTGOING.*\|.*BT_XSTACK_PHONEBOOK_MISSED' `
    'All category mask must OR the four public category bits'
foreach ($eventId in @(
        'STARTED','CONTACT','CALL','CATEGORY_COMPLETED','COMPLETED',
        'AUTH_REQUIRED','ERROR')) {
    Require $header ("BT_XSTACK_PHONEBOOK_EVENT_" + $eventId) `
        "Missing public PBAP event: $eventId"
}
Require $header 'uint8_t\s+type_flags\s*;' `
    'Phone-number type flags must be one byte'
Require $header 'uint8_t\s+direction\s*;' `
    'Call direction storage must be one byte'
Require $header 'uint8_t\s+truncated\s*;' `
    'Record truncation storage must be one byte'
foreach ($function in @(
        'bt_xstack_phonebook_sync', 'bt_xstack_phonebook_cancel',
        'bt_xstack_phonebook_authenticate', 'bt_xstack_phonebook_get_state')) {
    Require $header ("\b" + $function + "\s*\(") `
        "Missing public PBAP function: $function"
    Require $core ("\b" + $function + "\s*\(") `
        "Missing PBAP implementation: $function"
}
Require $header 'bt_xstack_phonebook_sync\s*\(\s*const\s+bt_xstack_addr_t\s*\*\s*address\s*,\s*bt_xstack_phonebook_category_mask_t\s+categories\s*\)' `
    'Phonebook sync must accept an address and public category mask'
foreach ($legacy in @(
        'BT_XSTACK_PBAP_PASSWORD_MAX', 'bt_xstack_pbap_state_t',
        'BT_XSTACK_PBAP_STATE_', 'bt_xstack_pbap_sync',
        'bt_xstack_pbap_cancel', 'bt_xstack_pbap_authenticate',
        'bt_xstack_get_pbap_state',
        'BT_XSTACK_PHONEBOOK_CATEGORY_MASK_CONTACTS',
        'BT_XSTACK_PHONEBOOK_CATEGORY_MASK_INCOMING',
        'BT_XSTACK_PHONEBOOK_CATEGORY_MASK_OUTGOING',
        'BT_XSTACK_PHONEBOOK_CATEGORY_MASK_MISSED',
        'BT_XSTACK_PHONEBOOK_CATEGORY_MASK_ALL')) {
    Forbid $header ("\b" + $legacy) "Legacy public PBAP symbol remains: $legacy"
}
Forbid $header '#include\s+["<](profiles/|adapter/|classic/)' `
    'Public phonebook API must not expose native/profile headers'
Forbid $header '\bbtstack_(pbap|vcard|bd_addr)' `
    'Public phonebook API must use only xStack-owned types'

Require $internal 'BT_XSTACK_REQUEST_PBAP_SYNC' 'Missing PBAP sync request'
Require $internal 'BT_XSTACK_REQUEST_PBAP_CANCEL' 'Missing PBAP cancel request'
Require $internal 'BT_XSTACK_REQUEST_PBAP_AUTHENTICATE' 'Missing PBAP auth request'
Require $internal 'BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED' `
    'Missing internal PBAP peer-disconnected request'
Require $internal 'BT_XSTACK_EVENT_DOMAIN_PHONEBOOK' `
    'Missing phonebook event domain'
Require $internal 'bt_xstack_phonebook_event_t\s+phonebook\s*;' `
    'Event envelope must own a deep-copied phonebook event'
Require $internal 'sizeof\s*\(\s*bt_xstack_phonebook_event_t\s*\)\s*<=\s*sizeof\s*\(\s*bt_xstack_audio_event_t\s*\)' `
    'Phonebook events must fit in the existing audio-sized envelope'

foreach ($native in @(
        'btstack_pbap_client_sync', 'btstack_pbap_client_cancel',
        'btstack_pbap_client_authenticate',
        'btstack_pbap_client_peer_disconnected',
        'btstack_pbap_client_get_state')) {
    Require $backend ("\b" + $native + "\s*\(") `
        "Backend does not execute generic PBAP wrapper: $native"
}
Require $manager '\bbtstack_pbap_client_set_event_callback\s*\(' `
    'PBAP generic event bridge is not registered'
Forbid $manager '\bbtstack_pbap_client_set_state_callback\s*\(' `
    'PBAP state must use the single structured event callback'
Require $events 'BT_XSTACK_EVENT_DOMAIN_PHONEBOOK' `
    'PBAP bridge must post phonebook-domain events'
Require $events '\bbtstack_core_state_read_pbap\s*\(' `
    'PBAP getter must read a protected cached state'
Require $manager 'manager_classic_state\.pbap_state' `
    'Profile Manager must own protected PBAP state'
Require $core 'btstack_core_state_read_pbap\s*\(' `
    'Public PBAP getter must use the cached-state reader'
Require $core 'length\s*>\s*BT_XSTACK_PHONEBOOK_PASSWORD_MAX' `
    'Password admission must use the public xStack capacity'
Require $core 'categories\s*==\s*0u' `
    'Phonebook sync must reject an empty category mask'
Require $core 'categories\s*&\s*\(?\s*bt_xstack_phonebook_category_mask_t\s*\)?\s*~BT_XSTACK_PHONEBOOK_ALL' `
    'Phonebook sync must reject category bits outside ALL'
Require $core 'request\.values\[0\]\s*=\s*categories\s*;' `
    'Phonebook sync must store categories in request value zero'
Require $core 'request\.value_count\s*=\s*1u\s*;' `
    'Phonebook sync must submit one request value'
Require $backend 'btstack_pbap_client_sync\s*\(\s*address\s*,\s*\(btstack_pbap_client_category_mask_t\)\s*request->values\[0\]\s*\)' `
    'Backend must forward the requested categories to generic PBAP'
Forbid $backend 'btstack_pbap_client_sync\s*\(\s*address\s*\)' `
    'Backend must not call the obsolete single-argument PBAP sync'
Forbid $core '\bbtstack_pbap_client_get_state\s*\(' `
    'Application-task getters must not touch the generic wrapper'

Require $profile '(?s)btstack_pbap_client_sync.*?state\s*!=\s*BTSTACK_PBAP_CLIENT_STATE_IDLE.*?BTSTACK_PBAP_CLIENT_EVENT_ERROR.*?BTSTACK_PBAP_CLIENT_STATUS_BUSY.*?memcpy\s*\(\s*event\.peer\s*,\s*address.*?deliver_event\s*\(\s*&event\s*\).*?return\s+BTSTACK_STATUS_BUSY' `
    'An admitted sync rejected by the run loop must emit one BUSY error for the requested peer'
Require $events '(?s)btstack_core_copy_address\s*\(\s*&envelope\.data\.phonebook\.address\s*,\s*event->peer\s*\).*?envelope\.data\.phonebook\.status\s*=.*?event->status.*?BTSTACK_PBAP_CLIENT_EVENT_ERROR\s*:.*?BT_XSTACK_PHONEBOOK_EVENT_ERROR' `
    'The PBAP bridge must preserve rejected peer/status and map the wrapper error to public ERROR'

Write-Output 'xStack PBAP public/request/state/event contract is complete'
