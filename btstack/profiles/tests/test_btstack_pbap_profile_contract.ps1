$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path

$headerPath = Join-Path $root 'btstack\include\profiles\btstack_pbap_client.h'
$parserHeaderPath = Join-Path $root 'btstack\include\profiles\btstack_vcard_parser.h'
$sourcePath = Join-Path $root 'btstack\profiles\btstack_pbap_client.c'
$parserSourcePath = Join-Path $root 'btstack\profiles\btstack_vcard_parser.c'
$classicPath = Join-Path $root 'btstack\core\btstack_profile_manager.c'
$configPath = Join-Path $root 'btstack\include\btstack_config.h'

foreach ($path in @($headerPath, $parserHeaderPath, $sourcePath, $parserSourcePath, $classicPath, $configPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing PBAP contract file: $path" }
}
$parserSource = Get-Content -Raw -LiteralPath $parserSourcePath
if ($parserSource -notmatch '#include\s+"profiles/btstack_vcard_parser.h"') {
    throw 'vCard parser source must use the public profiles include path'
}
if (Test-Path -LiteralPath (Join-Path $root 'btstack\profiles\btstack_vcard_parser.h')) {
    throw 'Private parser header must be removed after public include migration'
}

$header = Get-Content -Raw -LiteralPath $headerPath
if ($header -notmatch '#include\s+"profiles/btstack_vcard_parser.h"') {
    throw 'PBAP public header must use public profiles/vCard parser include'
}
foreach ($token in @(
    'btstack_pbap_client_setup', 'btstack_pbap_client_set_event_callback',
    'btstack_pbap_client_sync', 'btstack_pbap_client_cancel',
    'btstack_pbap_client_authenticate', 'btstack_pbap_client_get_state',
    'btstack_pbap_client_event_space_available',
    'btstack_pbap_client_peer_disconnected',
    'BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING', 'BTSTACK_PBAP_CLIENT_STATE_PAUSED',
    'BTSTACK_PBAP_CLIENT_CATEGORY_MISSED', 'BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED',
    'BTSTACK_PBAP_CLIENT_EVENT_CONNECTING',
    'BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATING',
    'BTSTACK_PBAP_CLIENT_EVENT_PULLING',
    'BTSTACK_PBAP_CLIENT_EVENT_PAUSED',
    'BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTING',
    'BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED',
    'BTSTACK_PBAP_CLIENT_STATUS_PARTIAL', 'BTSTACK_PBAP_CLIENT_STATUS_UNSUPPORTED',
    'BTSTACK_PBAP_CLIENT_STATUS_DENIED', 'BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED',
    'BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR', 'BTSTACK_PBAP_CLIENT_STATUS_CANCELED',
    'BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST', 'BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT',
    'BTSTACK_PBAP_CLIENT_STATUS_BUSY', 'parser_error_count', 'record', 'peer'
)) {
    if ($header -notmatch [regex]::Escape($token)) { throw "PBAP public contract missing: $token" }
}
if ($header -match 'btstack_pbap_client_set_state_callback|btstack_pbap_client_state_callback_t') {
    throw 'PBAP must expose one structured event callback only'
}

$source = Get-Content -Raw -LiteralPath $sourcePath
foreach ($token in @('classic/pbap_client.h', 'btstack_event.h', 'btstack_run_loop.h',
                      'classic/obex.h', 'OBEX_DISCONNECTED',
                      'PBAP_DATA_PACKET', 'pbap_set_flow_control_mode', 'pbap_next_packet',
                      'goep_client_disconnect', 'terminal_cleanup_started',
                      'pbap_subevent_connection_opened_get_pbap_cid',
                      'pbap_subevent_connection_closed_get_goep_cid',
                      'pbap_subevent_operation_completed_get_goep_cid',
                      'pbap_subevent_authentication_request_get_goep_cid',
                      'btstack_run_loop_execute_on_main_thread', 'PBAP_REPLAY_SIZE 1021u')) {
    if ($source -notmatch [regex]::Escape($token)) { throw "PBAP implementation contract missing: $token" }
}
$replayDeclarations = [regex]::Matches($source, '(?m)^\s*uint8_t\s+replay\s*\[')
if ($replayDeclarations.Count -ne 1) { throw "PBAP wrapper must have exactly one replay array; found $($replayDeclarations.Count)" }
$replayCapacity = [regex]::Match($source, '(?m)^#define\s+PBAP_REPLAY_SIZE\s+(\d+)u\s*$')
if (-not $replayCapacity.Success -or [int]$replayCapacity.Groups[1].Value -lt 1021) {
    throw 'PBAP replay fallback capacity must be at least 1021 bytes'
}
$goepDisconnectCalls = [regex]::Matches($source,
    '(?m)^\s*cleanup_status\s*=\s*goep_client_disconnect\s*\(\s*pbap\.cid\s*\)\s*;')
if ($goepDisconnectCalls.Count -ne 2) {
    throw 'All GOEP terminal cleanup must be owned by start_terminal_cleanup'
}
$ignoredGoepDisconnect = [regex]::Matches($source,
    '\(void\)\s*goep_client_disconnect\s*\(')
if ($ignoredGoepDisconnect.Count -ne 0) {
    throw 'PBAP wrapper must not ignore a GOEP disconnect result'
}

$stubBluetooth = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'pbap_host_stubs\bluetooth.h')
if ($stubBluetooth -notmatch '(?m)^#define\s+OBEX_DISCONNECTED\s+0xB2u\s*$') {
    throw 'Host stub OBEX_DISCONNECTED must match BTstack 1.6.2 value 0xB2'
}

$boundedFiles = @($headerPath, $parserHeaderPath, $sourcePath,
    (Join-Path $PSScriptRoot 'test_btstack_pbap_state_machine.c'),
    (Join-Path $PSScriptRoot 'test_btstack_pbap_state_machine.ps1'),
    (Join-Path $PSScriptRoot 'test_btstack_pbap_public_include.c'),
    (Join-Path $PSScriptRoot 'test_btstack_pbap_public_include.ps1')) +
    @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'pbap_host_stubs') -File -Recurse |
      ForEach-Object FullName)
$allocationPattern = '\b(malloc|calloc|realloc|free)\s*\('
$rtosPattern = '\b(xQueueCreate|xQueueCreateStatic|xTaskCreate|xTaskCreateStatic|QueueHandle_t|TaskHandle_t|StaticQueue_t|StaticTask_t)\b'
foreach ($boundedFile in $boundedFiles) {
    $text = Get-Content -Raw -LiteralPath $boundedFile
    if ($text -match $allocationPattern) { throw "PBAP file uses dynamic allocation: $boundedFile" }
    if ($text -match $rtosPattern) { throw "PBAP file creates or declares a FreeRTOS queue/task: $boundedFile" }
}

$classic = Get-Content -Raw -LiteralPath $classicPath
if ($classic -notmatch '#include\s+"profiles/btstack_pbap_client.h"' -or
    $classic -notmatch '\bbtstack_pbap_client_setup\s*\(\s*\)') {
    throw 'Profile Manager must initialize the generic PBAP wrapper'
}

$config = Get-Content -Raw -LiteralPath $configPath
if ($config -notmatch '(?m)^#define\s+BTSTACK_PBAP_CLIENT\s+1\s*$') {
    throw 'Canonical config must enable BTSTACK_PBAP_CLIENT'
}

Write-Output 'PASS: generic PBAP public/static/manager/config contract'
