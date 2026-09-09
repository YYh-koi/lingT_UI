$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$mediaPath = Join-Path $root 'btstack/profiles/btstack_a2dp_media.c'
$headerPath = Join-Path $root 'btstack/profiles/btstack_a2dp_media.h'
$sinkPath = Join-Path $root 'btstack/profiles/btstack_a2dp_sink.c'
$projectPath = Join-Path $root 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'
$makefilePath = Join-Path $root 'btstack/Makefile.inc'

foreach ($path in @($mediaPath, $headerPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing A2DP media module: $path"
    }
}

$media = Get-Content -Raw -LiteralPath $mediaPath
$header = Get-Content -Raw -LiteralPath $headerPath
$sink = Get-Content -Raw -LiteralPath $sinkPath
$project = Get-Content -Raw -LiteralPath $projectPath
$makefile = Get-Content -Raw -LiteralPath $makefilePath

foreach ($guard in @('RTP_VERSION', 'RTP_PADDING', 'RTP_EXTENSION',
        'RTP_CSRC_COUNT', 'SBC_FRAGMENTED')) {
    if ($media -notmatch $guard) {
        throw "Missing A2DP media guard: $guard"
    }
}
foreach ($api in @('btstack_a2dp_media_init', 'btstack_a2dp_media_configure',
        'btstack_a2dp_media_reset', 'btstack_a2dp_media_process')) {
    if ($header -notmatch $api) {
        throw "Missing A2DP media API: $api"
    }
}
if ($media -notmatch 'btstack_sbc_decoder_process_data') {
    throw 'A2DP media payload is not passed to the SBC decoder'
}
if ($media -notmatch 'BTSTACK_AUDIO_FORMAT_PCM_S16') {
    throw 'A2DP media decoder does not emit PCM S16'
}
if ($media -notmatch 'num_samples\s*\*\s*num_channels\s*\*\s*sizeof\s*\(\s*int16_t\s*\)') {
    throw 'PCM byte length does not account for frames and channels'
}
if ($sink -notmatch 'btstack_a2dp_media_process') {
    throw 'A2DP Sink media handler is not connected to the media module'
}
if ($sink -notmatch 'btstack_a2dp_media_configure') {
    throw 'SBC codec configuration is not connected to the media module'
}
if ($sink -notmatch 'BTSTACK_A2DP_PCM_CHANNEL_COUNT\s*=\s*2') {
    throw 'A2DP PCM configuration does not match the decoder stereo output'
}
if ($sink -notmatch 'btstack_a2dp_media_configure\s*\(\s*sample_rate\s*,\s*BTSTACK_A2DP_PCM_CHANNEL_COUNT\s*\)') {
    throw 'A2DP media configuration does not use the decoder PCM channel count'
}
if ($sink -notmatch 'btstack_a2dp_emit_pcm_config\s*\(\s*sample_rate\s*,\s*BTSTACK_A2DP_PCM_CHANNEL_COUNT\s*\)') {
    throw 'A2DP PCM event does not report the decoder PCM channel count'
}
if ($sink -notmatch 'btstack_a2dp_media_reset') {
    throw 'A2DP stream teardown does not reset media state'
}
if (($project -notmatch 'btstack_a2dp_media\.c') -or
    ($makefile -notmatch 'btstack_a2dp_media\.c')) {
    throw 'A2DP media module is missing from a library build description'
}

Write-Host 'BTstack A2DP RTP/SBC-to-PCM media contract is present'
