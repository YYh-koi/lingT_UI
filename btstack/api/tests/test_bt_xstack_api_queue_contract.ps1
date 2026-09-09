$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent $PSScriptRoot
$internalPath = Join-Path $apiRoot '..\core\btstack_core_internal.h'
$corePath = Join-Path $apiRoot 'bt_xstack_api_core.c'

if (-not (Test-Path -LiteralPath $internalPath)) {
    throw 'Missing API internal request model'
}
if (-not (Test-Path -LiteralPath $corePath)) {
    throw 'Missing API core implementation'
}

$internal = Get-Content -Raw -LiteralPath $internalPath
$core = Get-Content -Raw -LiteralPath $corePath

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) { throw $message }
}
function Assert-NoMatch([string]$text, [string]$pattern, [string]$message) {
    if ($text -match $pattern) { throw $message }
}

Assert-Match $internal 'BT_XSTACK_REQUEST_QUEUE_LEN\s+8u' `
    'Request queue length must remain eight'
Assert-Match $internal 'BT_XSTACK_EVENT_QUEUE_LEN\s+16u' `
    'Event queue length must remain sixteen'
Assert-Match $internal `
    'BT_XSTACK_COMMAND_SLOT_COUNT\s+BT_XSTACK_REQUEST_QUEUE_LEN' `
    'Command slot pool must cover every admitted request'
Assert-Match $internal 'char\s+text\s*\[\s*BT_XSTACK_TEXT_MAX\s*\]' `
    'Requests must own text storage'
Assert-Match $internal 'bt_xstack_addr_t\s+address\s*;' `
    'Requests must own address storage'

Assert-Match $core 'btstack_ring_t\s+bt_xstack_request_queue\s*;' `
    'API core must use the common fixed ring for requests'
Assert-Match $core 'btstack_core_request_t\s+bt_xstack_request_queue_storage\s*\[\s*BT_XSTACK_REQUEST_QUEUE_LEN\s*\]' `
    'Request ring storage must be a typed fixed array'
Assert-Match $core 'btstack_ring_init\s*\(\s*&bt_xstack_request_queue' `
    'Request ring must be initialized during API initialization'
Assert-Match $core 'btstack_platform_worker_start\s*\(' `
    'API core must start its worker through platform ops'
Assert-Match $core 'btstack_ring_push\s*\(\s*&bt_xstack_request_queue' `
    'API requests must use common ring submission'
Assert-Match $core 'memcpy\s*\(' `
    'API request values must be copied before queueing'
Assert-Match $core 'BT_XSTACK_BACKEND_BUSY' `
    'Worker must wait and retry when all prepared-command slots are occupied'
Assert-NoMatch $core 'BT_XSTACK_EXEC_TIMEOUT_MS' `
    'Individual commands must not rely on a global-failure completion timeout'
Assert-NoMatch $core 'FromISR' `
    'API front end must not expose an ISR submission path'
Assert-NoMatch $core '\bmalloc\s*\(' `
    'API core must not allocate from the heap'
Assert-NoMatch $core '\bfree\s*\(' `
    'API core must not allocate from the heap'
Assert-NoMatch $core '\bbtstack_at_' `
    'API core must not depend on the AT front end'
Assert-NoMatch $core '(?i)\b(?:StaticQueue_t|QueueHandle_t|xQueue\w*|uxQueue\w*)\b' `
    'API core must not depend on FreeRTOS queue types or calls'
Assert-NoMatch $core '#include\s*[<"]queue\.h[>"]' `
    'API core must not include the FreeRTOS queue header'
Assert-NoMatch $core '(?i)\b(?:TaskHandle_t|StaticTask_t|StackType_t|xTask\w*|ulTask\w*)\b' `
    'API core must not depend on FreeRTOS task types or calls'

Write-Output 'xStack API uses common fixed task-context rings'
