$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$projectPath = Join-Path $root 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'
$makefilePath = Join-Path $root 'btstack/Makefile.inc'
$project = Get-Content -Raw -LiteralPath $projectPath
$makefile = Get-Content -Raw -LiteralPath $makefilePath

$decoderSources = @(
    'alloc.c', 'bitalloc.c', 'bitalloc-sbc.c', 'bitstream-decode.c',
    'decoder-oina.c', 'decoder-private.c', 'decoder-sbc.c', 'dequant.c',
    'framing.c', 'framing-sbc.c', 'oi_codec_version.c', 'synthesis-sbc.c',
    'synthesis-dct8.c', 'synthesis-8-generated.c'
)

foreach ($source in $decoderSources) {
    $path = Join-Path $root "btstack/3rd-party/bluedroid/decoder/srce/$source"
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing SBC decoder source: $source"
    }
    if ($project -notmatch [regex]::Escape($source)) {
        throw "BTstack library project omits: $source"
    }
    if ($makefile -notmatch [regex]::Escape($source)) {
        throw "BTstack Makefile.inc omits: $source"
    }
}

foreach ($wrapper in @('btstack_sbc_decoder_bluedroid.c', 'btstack_sbc_plc.c')) {
    $path = Join-Path $root "btstack/src/classic/$wrapper"
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing SBC wrapper source: $wrapper"
    }
    if ($project -notmatch [regex]::Escape($wrapper)) {
        throw "BTstack library project omits: $wrapper"
    }
}

if ($project -notmatch '3rd-party[/\\]bluedroid[/\\]decoder[/\\]include') {
    throw 'BTstack library project omits the SBC decoder include directory'
}
if (-not (Test-Path -LiteralPath (Join-Path $root 'btstack/3rd-party/bluedroid/README.gpcv2178'))) {
    throw 'Missing pinned SBC decoder provenance record'
}

Write-Host 'BTstack SBC decoder sources and build integration are present'
