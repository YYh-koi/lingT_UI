$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$configPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
$queuePath = Join-Path $root 'btstack/core/btstack_event_queue.c'
$portPath = Join-Path $root 'btstack/ports/gpcv2178/btstack_gpcv2178_port.c'

foreach ($path in @($configPath, $queuePath, $portPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing PCM/disconnect diagnostic artifact: $path"
    }
}

$config = Get-Content -Raw -LiteralPath $configPath
$queue = Get-Content -Raw -LiteralPath $queuePath
$port = Get-Content -Raw -LiteralPath $portPath

if ($config -notmatch '#define\s+BTSTACK_EVENT_DATA_MAX\s+512u') {
    throw 'BTSTACK_EVENT_DATA_MAX does not accept a 512-byte SBC PCM frame'
}
if ($queue -notmatch 'sample->data_len\s*>\s*\(uint16_t\)BTSTACK_EVENT_DATA_MAX') {
    throw 'PCM payload validation does not accept the exact configured maximum'
}
if ($queue -match 'sample->data_len\s*>=\s*\(uint16_t\)BTSTACK_EVENT_DATA_MAX') {
    throw 'PCM payload validation incorrectly rejects an exact 512-byte frame'
}
foreach ($token in @(
        'HCI_EVENT_DISCONNECTION_COMPLETE',
        'hci_event_disconnection_complete_get_connection_handle',
        'hci_event_disconnection_complete_get_reason',
        'hci_add_event_handler')) {
    if ($port -notmatch $token) {
        throw "Missing disconnect-only HCI listener token: $token"
    }
}
if ($port -notmatch 'btstack disconnect handle=0x%04X reason=0x%02X\\r\\n') {
    throw 'Disconnect diagnostic does not print only handle and reason'
}
if ($port -notmatch '(?s)if\s*\(\s*!btstack_hci_event_registered\s*\).*?hci_add_event_handler\s*\(\s*&btstack_hci_event_callback\s*\).*?btstack_hci_event_registered\s*=\s*true') {
    throw 'Disconnect HCI listener is not registered exactly once'
}
if ($config -notmatch '#define\s+BTSTACK_GPCV2178_HCI_DUMP_ENABLE\s+0') {
    throw 'Full GPCV2178 HCI dump is not disabled'
}
if ($port -match 'HCI RX EVT len=|HCI %s ACL len=') {
    throw 'Disconnect diagnostic contains full HCI dump formatting'
}

Write-Host '512-byte PCM delivery and disconnect-only HCI diagnostics are configured'
