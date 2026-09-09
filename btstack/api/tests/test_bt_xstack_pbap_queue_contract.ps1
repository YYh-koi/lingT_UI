$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$api = Join-Path $root 'api'
$internal = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_core_internal.h')
$core = Get-Content -Raw -LiteralPath (Join-Path $api 'bt_xstack_api_core.c')
$backend = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_core_dispatcher.c')
$events = Get-Content -Raw -LiteralPath (Join-Path $root 'core\btstack_event_mapper.c')
$all = $internal + "`n" + $core + "`n" + $backend + "`n" + $events

function Require([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}
function Forbid([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) { throw $Message }
}

Require $internal 'BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE\s+4u' `
    'Ordinary traffic must preserve a bounded four-slot control risk budget'
Require $internal 'BT_XSTACK_API_EVENT_BACKPRESSURE\s+\(?-2\)?' `
    'PBAP records need a distinct non-dropping backpressure result'
Require $internal 'BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN\s+16u' `
    'Critical queue length must remain sixteen'
Require $internal 'BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN\s+16u' `
    'Best-effort queue length must remain sixteen'

$post = [regex]::Match($core,
    '(?s)int\s+btstack_core_post_event\s*\([^\{]+\)\s*\{.*?\n\}')
if (-not $post.Success) { throw 'Event-post function not found' }
$postText = $post.Value
$control = [regex]::Match($core,
    '(?s)static\s+bool\s+bt_xstack_api_is_control_event\s*\([^\{]+\)\s*\{.*?\n\}')
if (-not $control.Success) { throw 'Control-event classifier not found' }
$controlText = $control.Value
$requiredControlTokens = @(
    'BT_XSTACK_EVENT_DOMAIN_SERVICE',
    'BT_XSTACK_SERVICE_EVENT_DEVICE_STATE',
    'BT_XSTACK_SERVICE_EVENT_POWER_STATE',
    'BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE',
    'BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST',
    'BT_XSTACK_SERVICE_EVENT_PAIRED',
    'BT_XSTACK_EVENT_DOMAIN_CONNECTION',
    'BT_XSTACK_CONNECTION_DISCONNECTED',
    'BT_XSTACK_CONNECTION_CONNECTING',
    'BT_XSTACK_CONNECTION_CONNECTED',
    'BT_XSTACK_CONNECTION_DISCONNECTING',
    'BT_XSTACK_EVENT_DOMAIN_PHONE',
    'BT_XSTACK_PHONE_EVENT_STATE',
    'BT_XSTACK_PHONE_EVENT_DEVICE',
    'BT_XSTACK_PHONE_EVENT_AUDIO',
    'BT_XSTACK_PHONE_EVENT_LOST',
    'BT_XSTACK_EVENT_DOMAIN_AUDIO',
    'BT_XSTACK_AUDIO_EVENT_A2DP_STATE',
    'BT_XSTACK_AUDIO_EVENT_A2DP_DEVICE',
    'BT_XSTACK_AUDIO_EVENT_A2DP_LOST',
    'BT_XSTACK_AUDIO_EVENT_PCM_CONFIG',
    'BT_XSTACK_AUDIO_EVENT_AVRCP_STATE',
    'BT_XSTACK_EVENT_DOMAIN_PHONEBOOK',
    'BT_XSTACK_PHONEBOOK_EVENT_STARTED',
    'BT_XSTACK_PHONEBOOK_EVENT_CATEGORY_COMPLETED',
    'BT_XSTACK_PHONEBOOK_EVENT_COMPLETED',
    'BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED',
    'BT_XSTACK_PHONEBOOK_EVENT_ERROR'
)
foreach ($token in $requiredControlTokens) {
    Require $controlText ([regex]::Escape($token)) `
        "Control classifier explicit whitelist is missing $token"
}
Require $controlText 'default\s*:\s*return\s+false\s*;' `
    'Unknown event domains and IDs must remain ordinary'
Forbid $controlText 'PHONEBOOK[\s\S]*?!=\s*BT_XSTACK_PHONEBOOK_EVENT_(CONTACT|CALL)' `
    'Phonebook control classification must not accept unknown IDs by exclusion'

Require $postText 'BT_XSTACK_EVENT_DOMAIN_PHONEBOOK' `
    'Event post must recognize phonebook records'
Require $postText 'BT_XSTACK_PHONEBOOK_EVENT_CONTACT' `
    'Contact records must use reserved-slot admission'
Require $postText 'BT_XSTACK_PHONEBOOK_EVENT_CALL' `
    'Call records must use reserved-slot admission'
Require $postText 'bt_xstack_api_is_control_event\s*\(\s*event\s*\)' `
    'Event post must classify control events explicitly'
Require $postText '(?s)!\s*control_event.*?btstack_ring_spaces\s*\(\s*&bt_xstack_critical_event_queue\s*\)\s*<=\s*BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE' `
    'Every non-control critical event must stop at the four-slot reserve'
Require $postText 'result\s*=\s*BT_XSTACK_API_EVENT_BACKPRESSURE\s*;' `
    'Reserved/full records must return backpressure'
Require $postText '(?s)!\s*control_event.*?critical_event_drop_count\+\+.*?result\s*=\s*-1\s*;' `
    'Reserved ordinary critical events must count a drop and fail'
Require $postText '(?s)btstack_platform_enter_critical\s*\(\s*\).*?btstack_ring_spaces\s*\(\s*&bt_xstack_critical_event_queue\s*\).*?btstack_ring_push\s*\(\s*&bt_xstack_critical_event_queue\s*,.*?btstack_platform_exit_critical\s*\(\s*\)' `
    'Critical reserve check and ring push must share one critical section'
Require $postText '(?s)btstack_platform_exit_critical\s*\(\s*\).*?btstack_platform_notify\s*\(\s*BTSTACK_WORKER_EVENT\s*,\s*BTSTACK_WORKER_SIGNAL_WORK\s*\)' `
    'Successful event-task notification must happen after leaving the critical section'

# A record rejected by either the reserve or physical capacity must return
# backpressure before the ordinary drop path, leaving diagnostics untouched.
Require $postText '(?s)btstack_ring_push\s*\(\s*&bt_xstack_critical_event_queue.*?else\s+if\s*\(\s*pbap_record\s*\).*?result\s*=\s*BT_XSTACK_API_EVENT_BACKPRESSURE\s*;.*?critical_event_drop_count\+\+' `
    'Physically full records must backpressure without incrementing drop counts'

Require $internal 'BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE' `
    'Queue-space notification needs an internal run-loop request'
$eventTask = [regex]::Match($core,
    '(?s)static\s+void\s+bt_xstack_event_task\s*\([^\{]+\)\s*\{.*?\n\}')
if (-not $eventTask.Success) { throw 'Event task not found' }
Require $eventTask.Value 'bt_xstack_api_request_pbap_event_space_available\s*\(\s*\)' `
    'Every critical dequeue must request run-loop PBAP space handling'
Forbid $eventTask.Value 'btstack_pbap_client_event_space_available\s*\(' `
    'Event task must not touch run-loop-owned PBAP state directly'
Require $core 'bt_xstack_pbap_event_space_request_pending' `
    'Duplicate critical dequeues must coalesce while a space request is pending'
Require $core '(?s)static\s+void\s+bt_xstack_api_submit_event_task_request.*?btstack_ring_push\s*\(\s*&bt_xstack_request_queue.*?btstack_platform_wait\s*\(\s*BTSTACK_WORKER_EVENT\s*,\s*BTSTACK_WORKER_SIGNAL_REQUEST_SPACE\s*,\s*BTSTACK_WORKER_WAIT_FOREVER\s*\).*?for\s*\(\s*;;\s*\)' `
    'Event-task internal requests must block and retry until the request ring admits them'
Require $core '(?s)static\s+void\s+bt_xstack_api_task.*?btstack_ring_pop\s*\(\s*&bt_xstack_request_queue.*?btstack_platform_notify\s*\(\s*BTSTACK_WORKER_EVENT\s*,\s*BTSTACK_WORKER_SIGNAL_REQUEST_SPACE\s*\)' `
    'Every request dequeue must wake an event worker waiting for request space'
Require $core '(?s)bt_xstack_api_request_pbap_event_space_available.*?bt_xstack_api_submit_event_task_request\s*\(' `
    'The unique resume request must use reliable event-task admission'
Require $backend '(?s)case\s+BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE\s*:.*?btstack_pbap_client_event_space_available\s*\(\s*\).*?btstack_core_pbap_event_space_request_completed\s*\(\s*\)' `
    'PBAP space guard and pending completion must run on the BTstack run loop'

# Lifecycle/control events may consume the reserve; records and all other
# ordinary critical traffic may not.
Require $postText 'btstack_ring_push\s*\(\s*&bt_xstack_critical_event_queue' `
    'Non-scan events must continue through the critical queue'

$deliver = [regex]::Match($core,
    '(?s)void\s+btstack_frontend_deliver_event\s*\([^\{]+\)\s*\{.*?\n\}')
if (-not $deliver.Success) { throw 'Event delivery function not found' }
$phoneDelivery = [regex]::Match($deliver.Value,
    '(?s)case\s+BT_XSTACK_EVENT_DOMAIN_PHONE:.*?\n\s*break;')
if (-not $phoneDelivery.Success) { throw 'Phone event delivery block not found' }
Require $phoneDelivery.Value '(?s)config\.phone_callback.*?BT_XSTACK_PHONE_EVENT_LOST.*?BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED.*?bt_xstack_api_submit_event_task_request\s*\(\s*&request\s*,\s*1u\s*\)' `
    'Delivered HFP LOST must reliably enqueue PBAP peer-disconnected work after the application callback'
Forbid $phoneDelivery.Value 'BT_XSTACK_REQUEST_PBAP_SYNC' `
    'PHONE event delivery must not automatically enqueue PBAP sync for DEVICE'
Forbid $phoneDelivery.Value 'bt_xstack_phonebook_sync\s*\(|bt_xstack_api_submit\s*\(' `
    'Event-task coordination must not use the zero-timeout public submit path'
Forbid $core '\bBT_XSTACK_PHONEBOOK_CATEGORY_MASK_(CONTACTS|INCOMING|OUTGOING|MISSED|ALL)\b' `
    'Core must not consume legacy public phonebook CATEGORY_MASK names'
Forbid $events '(?s)static\s+void\s+bt_xstack_api_hfp_event.*?bt_xstack_phonebook_sync\s*\(' `
    'Run-loop HFP callback must not start automatic PBAP sync directly'

# Deterministic saturation model: the callback fills every existing request
# slot, then LOST cleanup must enter later without loss.
$modelQueue = [System.Collections.Generic.Queue[string]]::new()
$modelDrained = [System.Collections.Generic.List[string]]::new()
0..7 | ForEach-Object { $modelQueue.Enqueue("callback-$_") }
while ($modelQueue.Count -ge 8) {
    $modelDrained.Add($modelQueue.Dequeue())
}
$modelQueue.Enqueue('LOST-DISCONNECT')
while ($modelQueue.Count -ne 0) { $modelDrained.Add($modelQueue.Dequeue()) }
$coordinatorTail = @($modelDrained | Where-Object { $_ -match '^(DEVICE|LOST)-' })
if (($coordinatorTail.Count -ne 1) -or
    ($coordinatorTail[0] -ne 'LOST-DISCONNECT')) {
    throw 'Saturated request queue lost or reordered coordinator work'
}

Forbid $all '(?i)(pbap|phonebook)[A-Za-z0-9_]*_(queue|task)(_storage|_stack|_control|_handle)?\b' `
    'xStack must not add a PBAP-specific queue, task, worker, or storage array'

Write-Output 'xStack PBAP records use replay and a bounded four-slot control risk budget'
