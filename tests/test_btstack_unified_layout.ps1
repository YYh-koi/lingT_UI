$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$stack = Join-Path $root 'btstack'
$legacySplit = Join-Path $root 'btstack_api'

foreach ($directory in @('core', 'api', 'at', 'gap',
        'profiles', 'ports', 'src', 'transport')) {
    $path = Join-Path $stack $directory
    if (-not (Test-Path -LiteralPath $path -PathType Container)) {
        throw "Unified BTstack directory is missing: $path"
    }
}
$physicalAdapter = Join-Path $stack 'adapter'
if (Test-Path -LiteralPath $physicalAdapter) {
    throw 'Obsolete physical btstack/adapter directory must not remain'
}
$compatibilityDirectory = Join-Path $stack 'include\adapter'
$compatibilityHeaders = @(
    'btstack_adapter.h',
    'btstack_config.h',
    'btstack_port_ops.h',
    'btstack_property.h',
    'btstack_types.h'
)
foreach ($header in $compatibilityHeaders) {
    if (-not (Test-Path -LiteralPath (Join-Path $compatibilityDirectory $header) -PathType Leaf)) {
        throw "Application-neutral compatibility header is missing: $header"
    }
}
if (@(Get-ChildItem -LiteralPath $compatibilityDirectory -File).Count -ne
    $compatibilityHeaders.Count) {
    throw 'include/adapter must contain exactly the five compatibility headers'
}
if (@(Get-ChildItem -LiteralPath $stack -Recurse -File `
        -Filter 'bt_xstack_manager_internal.h').Count -ne 0) {
    throw 'Obsolete bt_xstack_manager_internal.h entity must not remain'
}
if (Test-Path -LiteralPath (Join-Path $stack 'events')) {
    throw 'Top-level events directory must be folded into core'
}
foreach ($source in @('btstack_event.c', 'btstack_event_queue.c')) {
    $path = Join-Path $stack (Join-Path 'core' $source)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Core event source is missing: $path"
    }
}
$atInternalPath = Join-Path $stack 'at\btstack_at_internal.h'
$atInternal = Get-Content -Raw -LiteralPath $atInternalPath
foreach ($token in @('BTSTACK_AT_TEXT_MAX', 'BTSTACK_AT_EVENT_DEVSTAT',
        'BTSTACK_AT_EVENT_ABSVOL')) {
    if ($atInternal -notmatch "\b$token\b") {
        throw "AT-owned event definition is missing: $token"
    }
}
if ($atInternal -match '\bBT_XSTACK_EVENT_(?:TEXT_MAX|ADDRESS_LEN|DEVSTAT|ABSVOL)\b') {
    throw 'AT event types still depend on the removed xStack router definitions'
}
$routerReferences = @(Get-ChildItem -LiteralPath $stack -Recurse -File -Include *.c,*.h |
    Select-String -Pattern '\bbt_xstack_event_router\b')
if ($routerReferences.Count -ne 0) {
    throw 'Unused bt_xstack_event_router must not remain in BTstack sources'
}
if (Test-Path -LiteralPath $legacySplit) {
    throw 'btstack_api must not remain after the unified-tree migration'
}

$runtimePath = Join-Path $stack 'core\btstack_runtime.c'
if (-not (Test-Path -LiteralPath $runtimePath -PathType Leaf)) {
    throw 'Neutral runtime composition root is missing'
}
$runtime = Get-Content -Raw -LiteralPath $runtimePath
foreach ($call in @('btstack_platform_get', 'btstack_core_backend_init')) {
    if ($runtime -notmatch "\b$call\s*\(") {
        throw "Runtime composition root does not own $call"
    }
}
if ($runtime -match '\bbtstack_gpcv2178_') {
    throw 'Neutral runtime composition root still calls a concrete GPCV2178 port'
}

$apiImplementation = Get-ChildItem -LiteralPath (Join-Path $stack 'api') `
    -Recurse -File -Include *.c,*.h |
    Where-Object { $_.FullName -notmatch '[\\/]tests[\\/]' } |
    ForEach-Object { Get-Content -Raw -LiteralPath $_.FullName }
$apiText = $apiImplementation -join "`n"
if ($apiText -match '\bbtstack_gpcv2178_(?:init|start_run_loop|run_loop_started)\s*\(') {
    throw 'API frontend directly owns the GPCV2178 lifecycle'
}

$forbiddenRoots = @('core', 'ports', 'gap', 'profiles')
$forbiddenPattern = @(
    '#include\s+["<]\.\./(?:api|at)/',
    '#include\s+["<]bt_xstack_api_internal\.h[">]',
    '#include\s+["<]btstack_at_internal\.h[">]',
    '\bbt_xstack_api_backend_init\s*\(',
    '\bbtstack_at_backend_init\s*\('
) -join '|'

foreach ($directory in $forbiddenRoots) {
    $path = Join-Path $stack $directory
    $files = Get-ChildItem -LiteralPath $path -Recurse -File -Include *.c,*.h
    foreach ($file in $files) {
        $text = Get-Content -Raw -LiteralPath $file.FullName
        if ($text -match $forbiddenPattern) {
            throw "Frontend dependency below the composition root: $($file.FullName)"
        }
    }
}

Write-Output 'Unified BTstack layout and dependency direction are valid'
