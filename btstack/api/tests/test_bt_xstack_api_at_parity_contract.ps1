$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent $PSScriptRoot
$core = Get-Content -Raw -LiteralPath (Join-Path $apiRoot 'bt_xstack_api_core.c')
$backend = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_core_dispatcher.c')
$events = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_event_mapper.c')
$manager = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_profile_manager.c')
$internal = Get-Content -Raw -LiteralPath (Join-Path $apiRoot '..\core\btstack_core_internal.h')

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) { throw $message }
}

Assert-Match $core 'config\s*>\s*7u' `
    'HFPCFG must preserve the AT range 0..7'
Assert-Match $core 'period_seconds\s*>\s*48u' `
    'SCAN must reject inquiry durations above 48 immediately'
Assert-Match $core 'length\s*>\s*31u' `
    'NAME must reject names above the native 31-byte limit immediately'
Assert-Match $manager 'BTSTACK_HFP_HF_CONTROL_ANSWER' `
    'HFPCFG auto-answer behavior is missing'
Assert-Match $manager 'btstack_avrcp_request_metadata\s*\(' `
    'Automatic AVRCP metadata behavior is missing'
Assert-Match $internal 'btstack_core_state_set_profile_peer\s*\(' `
    'Per-profile last-peer caching is missing'
Assert-Match $manager 'btstack_core_state_get_profile_peer\s*\(' `
    'Profile Manager connect-last must prefer the cached peer for its profile'
Assert-Match $backend 'btstack_profile_manager_connect\s*\(' `
    'Dispatcher must route connect and connect-last through Profile Manager'
Assert-Match $events 'power_on\)\s*value\s*\|=\s*1u' `
    'DEVSTAT bit 0 must represent power state'
Assert-Match $events 'discoverable\)\s*value\s*\|=\s*2u' `
    'DEVSTAT bit 1 must represent discoverable state'

Write-Output 'xStack typed API preserves stateful AT backend behavior'
