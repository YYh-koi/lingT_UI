$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$backendPath = Join-Path $root 'core\btstack_core_dispatcher.c'
$corePath = Join-Path $root 'api\bt_xstack_api_core.c'
$backend = Get-Content -LiteralPath $backendPath -Raw
$core = Get-Content -LiteralPath $corePath -Raw

function Assert-Contains {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text.IndexOf($Pattern, [System.StringComparison]::Ordinal) -lt 0) {
        throw $Message
    }
}

function Assert-NotContains {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text.IndexOf($Pattern, [System.StringComparison]::Ordinal) -ge 0) {
        throw $Message
    }
}

function Get-Section {
    param([string]$Text, [string]$Start, [string]$End)
    $startIndex = $Text.IndexOf($Start, [System.StringComparison]::Ordinal)
    if ($startIndex -lt 0) { throw "Missing section start: $Start" }
    $endIndex = $Text.IndexOf($End, $startIndex + $Start.Length,
        [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) { throw "Missing section end: $End" }
    return $Text.Substring($startIndex, $endIndex - $startIndex)
}

$local = Get-Section $backend `
    'static int btstack_core_execute_local_request' `
    'static int btstack_core_prepare_stack_request'
$prepare = Get-Section $backend `
    'static int btstack_core_prepare_stack_request' `
    'static int btstack_core_execute_stack_request'
$stack = Get-Section $backend `
    'static int btstack_core_execute_stack_request' `
    'static void btstack_core_execute'
$runLoopCallback = Get-Section $backend `
    'static void btstack_core_execute' `
    'int btstack_core_dispatch'
$dispatch = $backend.Substring($backend.IndexOf(
    'int btstack_core_dispatch',
    [System.StringComparison]::Ordinal))

$localRequests = @(
    'BT_XSTACK_REQUEST_SET_PROFILE_MASK',
    'BT_XSTACK_REQUEST_CLEAR_PEER_INDEX',
    'BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS',
    'BT_XSTACK_REQUEST_CLEAR_PEERS',
    'BT_XSTACK_REQUEST_SET_TDL',
    'BT_XSTACK_REQUEST_SET_HFP_CONFIG',
    'BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE',
    'BT_XSTACK_REQUEST_SET_A2DP_MUTE'
)
foreach ($requestId in $localRequests) {
    Assert-Contains $local $requestId `
        "$requestId must execute in API task context"
    Assert-NotContains $stack $requestId `
        "$requestId must not execute on the BTstack run loop"
}

Assert-Contains $prepare 'memcpy(prepared, request, sizeof(*prepared))' `
    'Request preparation must deep-copy the fixed request envelope'
foreach ($policy in @(
    'BT_XSTACK_REQUEST_SET_NAME',
    'BT_XSTACK_REQUEST_CONNECT_LAST',
    'BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO',
    'BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE',
    'btstack_core_event_set_scan_filter')) {
    Assert-NotContains $prepare $policy `
        "Request preparation must not own operation policy: $policy"
}

Assert-Contains $stack 'btstack_adapter_set_name_with_address_suffix' `
    'Adapter must own local-name suffix formatting'
Assert-Contains $stack 'btstack_profile_manager_connect' `
    'Profile Manager must own explicit and connect-last resolution'
Assert-Contains $stack 'btstack_hfp_hf_execute_control' `
    'HFP profile must own HFP control policy'
Assert-Contains $stack 'btstack_avrcp_execute_control' `
    'AVRCP profile must own play/pause policy'
Assert-Contains $stack 'btstack_core_event_set_scan_filter' `
    'Scan filter must be set at the scan operation boundary'

foreach ($forbidden in @(
    'strlen(',
    'btstack_core_copy_state',
    'btstack_core_state_get_profile_peer',
    'btstack_core_state_set_profile_peer')) {
    Assert-NotContains $stack $forbidden `
        "Run-loop stack executor contains preparation work: $forbidden"
}

$prepareIndex = $dispatch.IndexOf(
    'btstack_core_prepare_stack_request',
    [System.StringComparison]::Ordinal)
$scheduleIndex = $dispatch.IndexOf(
    'btstack_platform_post_stack_callback',
    [System.StringComparison]::Ordinal)
if (($prepareIndex -lt 0) -or ($scheduleIndex -lt 0) -or
    ($prepareIndex -ge $scheduleIndex)) {
    throw 'Stack request must be fully prepared before run-loop scheduling'
}
Assert-NotContains $backend 'btstack_run_loop_gpcv2178_enqueue_callback' `
    'API backend must schedule through the platform callback boundary'
Assert-NotContains $backend 'btstack_run_loop_gpcv2178_' `
    'API backend must not depend on concrete GPCV2178 run-loop functions'

Assert-NotContains $dispatch 'btstack_core_complete' `
    'Local and rejected requests must not synthesize command completion'
Assert-Contains $runLoopCallback 'btstack_core_execute_stack_request' `
    'Run-loop callback must invoke only the prepared stack executor'
Assert-NotContains $runLoopCallback 'btstack_core_state_sync_gap' `
    'Run-loop callback must not perform unconditional GAP synchronization'

$getterMatch = [regex]::Match($core,
    'bt_xstack_result_t\s+bt_xstack_get_avrcp_play_state\s*\([^\{]+\)\s*\{[^\}]+\}')
if (-not $getterMatch.Success) {
    throw 'AVRCP play-state Getter definition not found'
}
Assert-Contains $getterMatch.Value 'bt_xstack_api_read_avrcp_snapshot' `
    'AVRCP play-state Getter must use the compact protected AVRCP snapshot'
Assert-NotContains $getterMatch.Value 'bt_xstack_api_snapshot' `
    'AVRCP play-state Getter must not copy the complete cached state'
Assert-NotContains $getterMatch.Value 'bt_xstack_api_submit' `
    'Synchronous Getter must not schedule the BTstack run loop'

Write-Host 'xStack API prepares requests before the minimal run-loop boundary'
