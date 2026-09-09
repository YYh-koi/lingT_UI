param([string] $Compiler, [string] $CxxCompiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$gxx = if ($CxxCompiler) { $CxxCompiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\g++.exe') {
    'D:\mingw32\bin\g++.exe'
} else { (Get-Command g++.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$api = Split-Path -Parent $tests
$root = Split-Path -Parent $api
$build = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack_call_audio_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_bt_xstack_call_audio.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $includes = @(
        '-I', (Join-Path $root 'include\api'),
        '-I', (Join-Path $root 'include\adapter'),
        '-I', (Join-Path $root 'include'),
        '-I', (Join-Path $root 'core'),
        '-I', (Join-Path $root 'src'),
        '-I', (Join-Path $root 'transport'))

    & $gcc -std=c99 -Wall -Wextra -Werror @includes `
        (Join-Path $tests 'test_bt_xstack_call_audio.c') `
        (Join-Path $root 'core\btstack_call_audio_tx.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'Call-audio runtime compile failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'Call-audio runtime test failed' }

    $facadeExe = Join-Path $build 'test_btstack_hfp_audio_tx.exe'
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -DBTSTACK_CLASSIC=1 -DBTSTACK_HFP_HF=1 `
        -DHCI_ACL_PAYLOAD_SIZE=1021 @includes `
        -ffunction-sections -fdata-sections `
        (Join-Path $tests 'test_btstack_hfp_audio_tx.c') `
        (Join-Path $root 'profiles\btstack_hfp_hf.c') `
        '-Wl,--gc-sections' -o $facadeExe
    if ($LASTEXITCODE -ne 0) { throw 'HFP audio facade compile failed' }
    & $facadeExe
    if ($LASTEXITCODE -ne 0) { throw 'HFP audio facade runtime test failed' }

    $cHeader = Join-Path $tests 'test_bt_xstack_call_audio_header.c'
    $cppHeader = Join-Path $tests 'test_bt_xstack_call_audio_header.cpp'
    & $gcc -std=c99 -Wall -Wextra -Werror @includes -c $cHeader `
        -o (Join-Path $build 'header_c.o')
    if ($LASTEXITCODE -ne 0) { throw 'Public call-audio C99 header compile failed' }
    & $gxx -std=c++11 -Wall -Wextra -Werror @includes -c $cppHeader `
        -o (Join-Path $build 'header_cpp.o')
    if ($LASTEXITCODE -ne 0) { throw 'Public call-audio C++11 header compile failed' }

    $profile = Get-Content -Raw -LiteralPath `
        (Join-Path $root 'profiles\btstack_hfp_hf.c')
    foreach ($pattern in @(
            'HFP_SUBEVENT_AUDIO_CONNECTION_ESTABLISHED',
            'hfp_subevent_audio_connection_established_get_sco_handle',
            'hfp_subevent_audio_connection_established_get_tx_packet_length',
            'hfp_hf_set_direct_audio_callback',
            'hci_add_event_handler',
            'hci_can_send_sco_packet_now',
            'hci_reserve_packet_buffer',
            'hci_get_outgoing_packet_buffer',
            'hci_send_sco_packet_buffer')) {
        if ($profile -notmatch $pattern) {
            throw "HFP facade lacks SCO TX behavior: $pattern"
        }
    }
    foreach ($forbidden in @(
            'hci_register_sco_packet_handler',
            'hci_request_sco_can_send_now_event')) {
        if ($profile -match $forbidden) {
            throw "HFP facade must not own upstream SCO singleton: $forbidden"
        }
    }
    if ($profile -match '\b(?:malloc|calloc|realloc|free)\s*\(') {
        throw 'HFP audio path must not allocate dynamically'
    }

    $tx = Get-Content -Raw -LiteralPath `
        (Join-Path $root 'core\btstack_call_audio_tx.c')
    foreach ($pattern in @(
            'BT_XSTACK_CALL_AUDIO_TX_BYTES\s+2048u',
            'BT_XSTACK_CALL_AUDIO_WRITE_MAX\s+255u',
            'btstack_platform_enter_critical',
            'btstack_platform_exit_critical')) {
        if ($tx -notmatch $pattern) { throw "TX ring contract missing: $pattern" }
    }
    if ($tx -match '\b(?:malloc|calloc|realloc|free)\s*\(') {
        throw 'Call-audio TX ring must not allocate dynamically'
    }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'Application-owned call audio TX is bounded, FIFO, and session safe'
