param([string] $Compiler, [string] $CxxCompiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$gxx = if ($CxxCompiler) { $CxxCompiler } elseif (Test-Path -LiteralPath 'D:\mingw32\bin\g++.exe') {
    'D:\mingw32\bin\g++.exe'
} else { (Get-Command g++.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$api = Split-Path -Parent $tests
$root = Split-Path -Parent $api
$build = Join-Path ([IO.Path]::GetTempPath()) ('btstack_cover_header_' + [guid]::NewGuid().ToString('N'))

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $common = @('-I', (Join-Path $root 'include'), '-Wall', '-Wextra', '-Werror', '-fsyntax-only')
    $c = Join-Path $tests 'test_bt_xstack_cover_art_header.c'
    $cpp = Join-Path $tests 'test_bt_xstack_cover_art_header.cpp'
    $internal = Join-Path $root 'profiles\tests\test_btstack_cover_art_headers.c'

    & $gcc -std=c99 @common '-I' (Join-Path $root 'include\api') $c
    if ($LASTEXITCODE -ne 0) { throw 'API Cover Art C99 header compile failed' }
    & $gxx -std=c++11 @common '-I' (Join-Path $root 'include\api') $cpp
    if ($LASTEXITCODE -ne 0) { throw 'API Cover Art C++11 header compile failed' }
    & $gcc -std=c99 -DBTSTACK_COVER_ART_AT_HEADER=1 @common '-I' (Join-Path $root 'include\at') $c
    if ($LASTEXITCODE -ne 0) { throw 'AT Cover Art C99 header compile failed' }
    & $gxx -std=c++11 -DBTSTACK_COVER_ART_AT_HEADER=1 @common '-I' (Join-Path $root 'include\at') $cpp
    if ($LASTEXITCODE -ne 0) { throw 'AT Cover Art C++11 header compile failed' }
    & $gcc -std=c99 @common '-I' (Join-Path $root 'include\core') $internal
    if ($LASTEXITCODE -ne 0) { throw 'Internal Cover Art C99 header compile failed' }

    $coreText = Get-Content -Raw -LiteralPath (Join-Path $root 'api\bt_xstack_api_core.c')
    foreach ($token in @('bt_xstack_cover_art_set_mode', 'BT_XSTACK_REQUEST_COVER_ART_SET_MODE',
            'bt_xstack_cover_art_get_image', 'BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE',
            'btstack_frontend_deliver_cover_art', 'request.cover_art.descriptor = *descriptor')) {
        if ($coreText -notmatch [regex]::Escape($token)) { throw "API Cover Art integration missing: $token" }
    }
    if ($coreText -match 'request\.cover_art\.descriptor\s*=\s*descriptor\s*;') {
        throw 'API request must copy the descriptor, not retain its pointer'
    }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $tests 'test_bt_xstack_phonebook_api.ps1')
if ($LASTEXITCODE -ne 0) { throw 'Real-core API Cover Art behavior failed' }

Write-Output 'PASS: public headers and real-core API Cover Art behavior'
