$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$sourcePath = Join-Path $repoRoot 'btstack\src\classic\avrcp.c'
$source = Get-Content -Raw -LiteralPath $sourcePath
$controllerPath = Join-Path $repoRoot 'btstack\profiles\btstack_avrcp_controller.c'
$controller = Get-Content -Raw -LiteralPath $controllerPath

$requirements = @(
    @('uint16_t\s+target_supported_features\s*;',
      'AVRCP SDP context must retain remote Target Supported Features'),
    @('case\s+BLUETOOTH_ATTRIBUTE_SUPPORTED_FEATURES\s*:',
      'AVRCP SDP parser must handle Supported Features attribute 0x0311'),
    @('de_element_get_uint16\s*\([^;]*target_supported_features\s*\)',
      'AVRCP SDP parser must decode Target Supported Features'),
    @('target_supported_features\s*=\s*0\s*;',
      'AVRCP SDP query must clear Target Supported Features before reuse'),
    @('cover_art_l2cap_psm\s*=\s*0\s*;',
      'AVRCP SDP query must clear Cover Art PSM before reuse'),
    @('btstack avrcp: sdp status=.*target_features=.*target_cover_art=.*cover_art_psm=.*psm_available=.*cover_art=',
      'AVRCP SDP completion must distinguish the Target feature bit from Cover Art PSM availability'),
    @('AVRCP_TARGET_SUPPORTED_FEATURE_COVER_ART',
      'Cover Art diagnostics must use the named AVRCP Target feature bit'),
    @('\(status\s*==\s*ERROR_CODE_SUCCESS\s*&&\s*avrcp_sdp_query_context\.cover_art_l2cap_psm\s*!=\s*0u\)\s*\?\s*1u\s*:\s*0u',
      'Cover Art compatibility verdict must accept a valid PSM even if a peer omits the Target feature bit')
)

foreach ($requirement in $requirements) {
    if ($source -notmatch $requirement[0]) {
        throw $requirement[1]
    }
}

foreach ($feature in @(
    'AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_IMAGE_PROPERTIES',
    'AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_IMAGE',
    'AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_LINKED_THUMBNAIL'
)) {
    $pattern = 'controller_features\s*\|=\s*\(uint16_t\)\(1u\s*<<\s*' +
        [regex]::Escape($feature) + '\s*\)\s*;'
    if ($controller -notmatch $pattern) {
        throw "AVRCP Controller SDP must advertise $feature when Cover Art is enabled"
    }
}

if ($controller -notmatch '(?s)#if\s+BTSTACK_AVRCP_COVER_ART.*?COVER_ART_GET_IMAGE_PROPERTIES.*?COVER_ART_GET_IMAGE.*?COVER_ART_GET_LINKED_THUMBNAIL.*?#endif') {
    throw 'AVRCP Controller Cover Art SDP features must remain guarded by BTSTACK_AVRCP_COVER_ART'
}

Write-Output 'PASS: AVRCP SDP Cover Art diagnostics contract'
