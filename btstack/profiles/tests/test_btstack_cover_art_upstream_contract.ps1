$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourceRelative = 'btstack/src/classic/avrcp_cover_art_client.c'
$source = Join-Path $repoRoot $sourceRelative

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw 'missing native AVRCP Cover Art client source'
}

$sourceText = Get-Content -Raw -LiteralPath $source
$requiredNativeTokens = @(
    'avrcp_cover_art_client_connect',
    'avrcp_cover_art_client_is_available',
    'avrcp_cover_art_client_get_linked_thumbnail',
    'avrcp_cover_art_client_get_image_properties',
    'avrcp_cover_art_client_get_image',
    'BIP_DATA_PACKET',
    'ENABLE_AVRCP_COVER_ART'
)
foreach ($token in $requiredNativeTokens) {
    if ($sourceText -notmatch [regex]::Escape($token)) {
        throw "native AVRCP Cover Art source missing $token"
    }
}

if ($sourceText -notmatch '(?s)avrcp_cover_art_client_is_available\s*\([^)]*\).*?cover_art_psm\s*!=\s*0') {
    throw 'native Cover Art availability must use the AVRCP SDP Cover Art PSM'
}

$configPath = Join-Path $repoRoot 'btstack/include/btstack_config.h'
$config = Get-Content -Raw -LiteralPath $configPath
if ($config -notmatch '(?m)^#define\s+BTSTACK_AVRCP_COVER_ART\s+1\s*$') {
    throw 'missing canonical Cover Art switch'
}
if ($config -notmatch '(?s)#if\s+BTSTACK_CLASSIC\s*&&.*?BTSTACK_AVRCP_CONTROLLER\s*&&.*?BTSTACK_AVRCP_TARGET\s*&&.*?BTSTACK_AVRCP_COVER_ART.*?#define\s+ENABLE_AVRCP_COVER_ART') {
    throw 'native ENABLE_AVRCP_COVER_ART derivation lacks parent guards'
}

$projects = @{
    API = 'Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj'
    AT = 'Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj'
    Legacy = 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'
}
foreach ($name in @('API', 'AT')) {
    $text = Get-Content -Raw -LiteralPath (Join-Path $repoRoot $projects[$name])
    $count = ([regex]::Matches($text, 'btstack/src/classic/avrcp_cover_art_client\.c')).Count
    if ($count -ne 1) {
        throw "$name project must contain native Cover Art source exactly once; actual=$count"
    }
    $bipCount = ([regex]::Matches($text, 'btstack/profiles/btstack_bip_client\.c')).Count
    if ($bipCount -ne 1) {
        throw "$name project must contain BIP wrapper exactly once; actual=$bipCount"
    }
    $coordinatorCount = ([regex]::Matches($text, 'btstack/profiles/btstack_avrcp_cover_art\.c')).Count
    if ($coordinatorCount -ne 1) {
        throw "$name project must contain Cover Art coordinator exactly once; actual=$coordinatorCount"
    }
    $xmlCount = ([regex]::Matches($text, 'btstack/3rd-party/yxml/yxml\.c')).Count
    if ($xmlCount -ne 1) {
        throw "$name project must contain shared yxml exactly once; actual=$xmlCount"
    }
}
$legacyText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot $projects.Legacy)
if ($legacyText -match 'btstack/src/classic/avrcp_cover_art_client\.c') {
    throw 'Legacy project must remain unchanged by Cover Art integration'
}
if ($legacyText -match 'btstack/profiles/btstack_bip_client\.c') {
    throw 'Legacy project must not contain the new BIP wrapper'
}
if ($legacyText -match 'btstack/profiles/btstack_avrcp_cover_art\.c') {
    throw 'Legacy project must not contain the new Cover Art coordinator'
}

$makeText = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'btstack/Makefile.inc')
if (([regex]::Matches($makeText, '(?m)^BTSTACK_XML_SOURCES\s*:=' )).Count -ne 1 -or
    ([regex]::Matches($makeText, '\$\(BTSTACK_ROOT\)/3rd-party/yxml/yxml\.c')).Count -ne 1) {
    throw 'yxml must be owned by one shared XML source group'
}

Write-Output 'PASS: native AVRCP Cover Art client and selected build boundary'
