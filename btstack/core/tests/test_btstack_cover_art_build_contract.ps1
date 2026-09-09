$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$bipPath = Join-Path $root 'btstack/profiles/btstack_bip_client.c'
$coordinatorPath = Join-Path $root 'btstack/profiles/btstack_avrcp_cover_art.c'
$mapperPath = Join-Path $root 'btstack/core/btstack_event_mapper.c'
$apiFrontendPath = Join-Path $root 'btstack/api/bt_xstack_api_core.c'
$atFrontendPath = Join-Path $root 'btstack/at/bt_xstack_at_frontend.c'
$projects = @(
    @{ Name = 'API'; Path = Join-Path $root 'Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj'; Expected = 1 },
    @{ Name = 'AT'; Path = Join-Path $root 'Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj'; Expected = 1 },
    @{ Name = 'Legacy'; Path = Join-Path $root 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'; Expected = 0 }
)

foreach ($path in @($bipPath, $coordinatorPath, $mapperPath,
        $apiFrontendPath, $atFrontendPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing Cover Art production source: $path"
    }
}

$units = @(
    'btstack/src/classic/avrcp_cover_art_client.c',
    'btstack/profiles/btstack_bip_client.c',
    'btstack/profiles/btstack_avrcp_cover_art.c'
)
foreach ($project in $projects) {
    $text = (Get-Content -Raw -LiteralPath $project.Path).Replace('\', '/')
    foreach ($unit in $units) {
        $leaf = [IO.Path]::GetFileName($unit)
        $actual = [regex]::Matches($text, [regex]::Escape($leaf)).Count
        if ($actual -ne $project.Expected) {
            throw "$($project.Name) must contain $unit exactly $($project.Expected) time(s), actual=$actual"
        }
    }
}

$production = (Get-Content -Raw -LiteralPath $bipPath) +
    (Get-Content -Raw -LiteralPath $coordinatorPath)
if ($production -match '\b(malloc|calloc|realloc|free)\s*\(') {
    throw 'Cover Art/BIP must not allocate dynamically'
}
$bip = Get-Content -Raw -LiteralPath $bipPath
foreach ($required in @(
        'uint8_t ertm_buffer[2000]',
        'char descriptor[160]',
        'btstack_bip_reset_connection',
        'btstack_bip_client.operation == operation',
        'btstack_bip_client.generation == generation')) {
    if ($bip -notmatch [regex]::Escape($required)) {
        throw "BIP hardening contract missing: $required"
    }
}
$coordinator = Get-Content -Raw -LiteralPath $coordinatorPath
if ($coordinator -notmatch 'event->generation\s*!=\s*btstack_avrcp_cover_art\.active_generation') {
    throw 'Coordinator must suppress stale generations before frontend delivery'
}
$mapper = Get-Content -Raw -LiteralPath $mapperPath
$coverFunction = [regex]::Match($mapper,
    '(?s)void\s+btstack_core_event_mapper_cover_art\s*\([^)]*\)\s*\{(?<body>.*?)\n\}')
if (-not $coverFunction.Success) {
    throw 'Cover Art mapper is missing'
}
if ($coverFunction.Groups['body'].Value -match
    'btstack_(core|at)_(post_event|event_ring)') {
    throw 'Cover Art bytes must not enter API or AT event rings'
}
foreach ($frontendPath in @($apiFrontendPath, $atFrontendPath)) {
    $frontend = Get-Content -Raw -LiteralPath $frontendPath
    if ($frontend -notmatch 'btstack_frontend_deliver_cover_art' -or
        $frontend -notmatch 'btstack_platform_exit_critical\s*\(\s*\)') {
        throw "Frontend must snapshot then deliver Cover Art outside critical: $frontendPath"
    }
}

Write-Output 'Cover Art/BIP build, memory, stale-generation, and direct-callback contracts pass'
