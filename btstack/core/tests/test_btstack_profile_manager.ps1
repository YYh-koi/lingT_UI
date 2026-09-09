param([string] $Compiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$core = Split-Path -Parent $tests
$root = Split-Path -Parent $core
$build = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack_profile_manager_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_profile_manager.exe'
$mapperExe = Join-Path $build 'test_btstack_cover_art_mapper.exe'
$managerSource = Join-Path $core 'btstack_profile_manager.c'
$mapperSource = Join-Path $core 'btstack_event_mapper.c'

$profileHeaders = @(
    (Join-Path $root 'include\profiles\btstack_a2dp_sink.h'),
    (Join-Path $root 'include\profiles\btstack_hfp_hf.h'),
    (Join-Path $root 'include\profiles\btstack_avrcp.h'),
    (Join-Path $root 'include\profiles\btstack_pbap_client.h'))
$profileHeaderText = ($profileHeaders | ForEach-Object {
        Get-Content -Raw -LiteralPath $_
    }) -join "`n"
foreach ($requiredEvent in @(
        'BTSTACK_A2DP_SINK_EVENT_CONNECTING',
        'BTSTACK_A2DP_SINK_EVENT_CONNECTED',
        'BTSTACK_A2DP_SINK_EVENT_STREAM_CLOSED',
        'BTSTACK_A2DP_SINK_EVENT_DISCONNECTED',
        'BTSTACK_HFP_HF_EVENT_CONNECTING',
        'BTSTACK_HFP_HF_EVENT_CONNECTED',
        'BTSTACK_HFP_HF_EVENT_CALL_IDLE',
        'BTSTACK_HFP_HF_EVENT_DISCONNECTED',
        'BTSTACK_AVRCP_EVENT_CONNECTING',
        'BTSTACK_AVRCP_EVENT_CONNECTED',
        'BTSTACK_AVRCP_EVENT_DISCONNECTED',
        'BTSTACK_PBAP_CLIENT_EVENT_CONNECTING',
        'BTSTACK_PBAP_CLIENT_EVENT_PULLING',
        'BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED')) {
    if ($profileHeaderText -notmatch [regex]::Escape($requiredEvent)) {
        throw "Profile state transition is not an explicit event: $requiredEvent"
    }
}
foreach ($obsoleteEvent in @(
        'BTSTACK_A2DP_SINK_EVENT_STATE',
        'BTSTACK_HFP_HF_EVENT_STATE',
        'BTSTACK_AVRCP_EVENT_STATE',
        'BTSTACK_PBAP_CLIENT_EVENT_STATE')) {
    if ($profileHeaderText -match [regex]::Escape($obsoleteEvent)) {
        throw "Profile still exposes a generic state event: $obsoleteEvent"
    }
}

$managerText = Get-Content -Raw -LiteralPath $managerSource
$mapperText = Get-Content -Raw -LiteralPath $mapperSource
$dispatcherText = Get-Content -Raw -LiteralPath (Join-Path $core 'btstack_core_dispatcher.c')
$internalText = Get-Content -Raw -LiteralPath (Join-Path $core 'btstack_core_internal.h')
foreach ($binding in @(
        'btstack_pbap_client_set_event_callback',
        'btstack_avrcp_cover_art_setup',
        'a2dp_config.event_callback', 'avrcp_config.event_callback',
        'hfp_config.event_callback')) {
    if ($managerText -notmatch [regex]::Escape($binding)) {
        throw "Profile Manager missing Classic subscription: $binding"
    }
}
foreach ($obsolete in @(
        'btstack_pbap_client_set_state_callback',
        'btstack_a2dp_sink_set_state_callback',
        'profile_event_callback')) {
    if ($managerText -match [regex]::Escape($obsolete)) {
        throw "Profile Manager still uses duplicate callback surface: $obsolete"
    }
}
foreach ($policy in @(
        'btstack_hfp_hf_control', 'btstack_avrcp_connect',
        'btstack_avrcp_request_metadata',
        'btstack_call_audio_tx_session_changed')) {
    if ($managerText -notmatch ("\b" + [regex]::Escape($policy) + '\s*\(')) {
        throw "Profile Manager missing Classic policy: $policy"
    }
    if ($mapperText -match ("\b" + [regex]::Escape($policy) + '\s*\(')) {
        throw "Event Mapper still owns Classic policy: $policy"
    }
}
if ($mapperText -match '\bbtstack_(hfp_hf_set_event_callback|pbap_client_set_event_callback|a2dp_sink_set_event_callback|avrcp_set_event_callback)\s*\(') {
    throw 'Event Mapper still owns a Classic profile subscription'
}
foreach ($token in @('BT_XSTACK_REQUEST_COVER_ART_SET_MODE', 'BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE',
        'btstack_avrcp_cover_art_set_mode', 'btstack_avrcp_cover_art_get_image')) {
    if ($dispatcherText -notmatch [regex]::Escape($token)) { throw "Dispatcher missing Cover Art route: $token" }
}
if ($mapperText -notmatch 'void\s+btstack_core_event_mapper_cover_art\s*\(' -or
    $mapperText -notmatch 'btstack_frontend_deliver_cover_art\s*\(') {
    throw 'Event Mapper missing direct Cover Art delivery'
}
$coverMapper = [regex]::Match($mapperText, '(?s)void\s+btstack_core_event_mapper_cover_art\s*\([^)]*\)\s*\{(?<body>.*?)\n\}').Groups['body'].Value
if ($coverMapper -match '\bbtstack_core_post_event\s*\(') { throw 'Cover Art must not enter the ordinary event ring' }
$envelope = [regex]::Match($internalText, '(?s)typedef\s+struct\s*\{\s*btstack_core_event_domain_t\s+domain;.*?btstack_core_event_envelope_t;').Value
if ($envelope -match '(?i)cover_art|btstack_bip') { throw 'Cover Art payload must not inflate the fixed event envelope' }
foreach ($removed in @(
        (Join-Path $root 'profiles\btstack_classic_profiles.c'),
        (Join-Path $root 'include\profiles\btstack_classic_profiles.h'))) {
    if (Test-Path -LiteralPath $removed) {
        throw "Obsolete Classic composition still exists: $removed"
    }
}

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $includes = @(
        '-I', (Join-Path $root 'include'),
        '-I', (Join-Path $root 'include\adapter'),
        '-I', (Join-Path $root 'src'),
        '-I', (Join-Path $root 'transport'),
        '-I', (Join-Path $root 'core'))
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -DENABLE_CLASSIC -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -DBTSTACK_GATT_CLIENT=1 -DBTSTACK_GATT_SERVER=1 -DBTSTACK_HOGP_DEVICE=0 @includes `
        $managerSource `
        (Join-Path $tests 'test_btstack_profile_manager.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'Profile Manager host test compile failed'
    }
    foreach ($scenario in @(
            'null', 'short', 'le', 'le-invalid', 'success', 'idempotent',
            'events', 'operations', 'linkage-disabled',
            'pbap-fail', 'a2dp-fail', 'avrcp-fail', 'hfp-fail')) {
        & $exe $scenario
        if ($LASTEXITCODE -ne 0) {
            throw "Profile Manager scenario failed: $scenario"
        }
    }
    & $gcc -std=c99 -Wall -Wextra -Werror -ffunction-sections -fdata-sections @includes `
        $mapperSource (Join-Path $tests 'test_btstack_cover_art_mapper.c') `
        '-Wl,--gc-sections' -o $mapperExe
    if ($LASTEXITCODE -ne 0) { throw 'Cover Art mapper host test compile failed' }
    & $mapperExe
    if ($LASTEXITCODE -ne 0) { throw 'Cover Art mapper host test failed' }
}
finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'BTstack Profile Manager lifecycle and Classic composition pass'
