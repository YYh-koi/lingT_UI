$ErrorActionPreference = 'Stop'

$apiRoot = Split-Path -Parent $PSScriptRoot
$stackRoot = Split-Path -Parent $apiRoot
$hciHeader = Get-Content -Raw -LiteralPath (Join-Path $stackRoot 'src\hci.h')
$hfpSource = Get-Content -Raw -LiteralPath (
    Join-Path $stackRoot 'src\classic\hfp_hf.c')

if ($hciHeader -notmatch '(?s)READ_SCO_PACKET_STATUS.*?>>\s*12.*?0x03u') {
    throw 'Missing bounded SCO Packet Status accessor'
}
$handlerMatch = [regex]::Match(
    $hfpSource,
    '(?s)static\s+void\s+hfp_hf_sco_packet_handler\s*\(.*?(?=void\s+hfp_hf_set_direct_audio_callback)')
if (-not $handlerMatch.Success) {
    throw 'Cannot locate direct SCO packet handler'
}
$handler = $handlerMatch.Value
foreach ($pattern in @(
        'payload_length\s*=\s*packet\[2\]',
        'size\s*<\s*\(uint16_t\)\s*\(HCI_SCO_HEADER_SIZE\s*\+\s*payload_length\)',
        'READ_SCO_PACKET_STATUS\s*\(\s*packet\s*\)',
        'memset\s*\(\s*&packet\[HCI_SCO_HEADER_SIZE\]\s*,\s*0\s*,\s*payload_length\s*\)',
        'hfp_hf_direct_audio_callback\s*\(\s*&packet\[HCI_SCO_HEADER_SIZE\]\s*,\s*payload_length\s*\)')) {
    if ($handler -notmatch $pattern) {
        throw "Missing safe SCO delivery behavior: $pattern"
    }
}

Write-Output 'Erroneous SCO packets are replaced with bounded silence'
