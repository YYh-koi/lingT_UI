$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$portPath = Join-Path $repoRoot 'btstack\ports\gpcv2178\btstack_gpcv2178_port.c'
$projectPath = Join-Path $repoRoot 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj'
$buildScriptPath = Join-Path $repoRoot 'Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1'

function Assert-Match([string]$text, [string]$pattern, [string]$message) {
    if ($text -notmatch $pattern) { throw $message }
}

function Assert-NoMatch([string]$text, [string]$pattern, [string]$message) {
    if ($text -match $pattern) { throw $message }
}

if (-not (Test-Path -LiteralPath $portPath)) {
    throw 'Missing copied GPCV2178 port'
}
if (-not (Test-Path -LiteralPath $projectPath)) {
    throw 'Missing independent BTstack API library project'
}
if (-not (Test-Path -LiteralPath $buildScriptPath)) {
    throw 'Missing BTstack API build script'
}

$port = Get-Content -Raw -LiteralPath $portPath
$project = Get-Content -Raw -LiteralPath $projectPath
$buildScript = Get-Content -Raw -LiteralPath $buildScriptPath

Assert-NoMatch $port '#include\s+"(?:bt_xstack_api_internal|btstack_at_internal)\.h"' `
    'Platform port must not include a frontend-private contract'
Assert-NoMatch $port '\b(?:bt_xstack_api_backend_init|btstack_at_backend_init)\s*\(' `
    'Platform port must not initialize a frontend backend'
Assert-NoMatch $port '\bbtstack_at_' `
    'Copied port must not initialize the AT backend'

Assert-Match $project 'title="BTstack_API_Library"' `
    'API project must have an independent project title'
Assert-Match $project 'bt_api/xstack_api' `
    'API project must emit libxstack_api.a under bt_api'
foreach ($source in @('bt_xstack_api_core.c','btstack_core_dispatcher.c',
        'btstack_event_mapper.c','btstack_runtime.c')) {
    Assert-Match $project ([regex]::Escape($source)) "API project is missing $source"
}
Assert-NoMatch $project 'btstack/at/' `
    'API project must not compile or include the copied AT layer'
Assert-NoMatch $project 'btstack/adapter/bt_xstack_manager\.c' `
    'API project must not compile the legacy AT-facing manager'
Assert-Match $project 'filename="\.\./\.\./\.\./btstack/' `
    'API project source units must come from the unified btstack tree'
Assert-NoMatch $project 'btstack_api/' `
    'API project still references the removed split source tree'
Assert-Match $buildScript 'Remove-Item\s+-LiteralPath\s+\$library\s+-Force' `
    'Build script must remove the exact old archive before rebuilding'
Assert-Match $buildScript '&\s+\$MakeProgram\s+-B\s+-f\s+\$makeFile' `
    'Build script must force recompilation so header changes are not stale'

Write-Output 'xStack API port and independent library project are wired without AT'
