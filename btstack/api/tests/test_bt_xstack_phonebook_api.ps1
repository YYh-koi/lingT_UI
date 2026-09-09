$ErrorActionPreference = 'Stop'

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$hostBin = 'D:\mingw32\bin'
$armBin = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$hostGcc = Join-Path $hostBin 'gcc.exe'
$hostGxx = Join-Path $hostBin 'g++.exe'
$armGcc = Join-Path $armBin 'arm-none-eabi-gcc.exe'
$armGxx = Join-Path $armBin 'arm-none-eabi-g++.exe'
$stubInclude = Join-Path $PSScriptRoot 'stubs'
$apiInclude = Join-Path $root 'include\api'
$core = Join-Path $root 'api\bt_xstack_api_core.c'
$test = Join-Path $PSScriptRoot 'test_bt_xstack_phonebook_api.c'
$cFixture = Join-Path $PSScriptRoot 'test_bt_xstack_api_headers.c'
$cppFixture = Join-Path $PSScriptRoot 'test_bt_xstack_api_headers.cpp'
$build = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('bt_xstack_phonebook_api_' + [System.Guid]::NewGuid().ToString('N'))

foreach ($tool in @($hostGcc, $hostGxx, $armGcc, $armGxx)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Required compiler not found: $tool"
    }
}

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $runtime = Join-Path $build 'test_bt_xstack_phonebook_api.exe'
    & $hostGcc -std=c11 -Wall -Wextra -Werror `
        -ffunction-sections -fdata-sections `
        -I $stubInclude -I $apiInclude -I (Join-Path $root 'api') `
        -I (Join-Path $root 'include') `
        -I (Join-Path $root 'ports\gpcv2178') `
        -I (Join-Path $root 'src') -I (Join-Path $root 'transport') `
        -I (Join-Path $root 'core') `
        $core (Join-Path $root 'core\btstack_ring.c') $test `
        '-Wl,--gc-sections' -o $runtime
    if ($LASTEXITCODE -ne 0) { throw 'Host phonebook API test compile failed' }
    & $runtime
    if ($LASTEXITCODE -ne 0) { throw 'Host phonebook API behavior test failed' }
    & $runtime 'null-cover'
    if ($LASTEXITCODE -ne 0) { throw 'Host NULL Cover Art callback behavior failed' }

    & $hostGcc -std=c11 -Wall -Wextra -Werror -fsyntax-only `
        -I $apiInclude $cFixture
    if ($LASTEXITCODE -ne 0) { throw 'Host C public header fixture failed' }
    & $hostGxx -std=c++11 -Wall -Wextra -Werror -fsyntax-only `
        -I $apiInclude $cppFixture
    if ($LASTEXITCODE -ne 0) { throw 'Host C++ public header fixture failed' }

    & $armGcc -std=c99 -Wall -Wextra -Werror -fsyntax-only `
        -I $apiInclude $cFixture
    if ($LASTEXITCODE -ne 0) { throw 'ARM C public header fixture failed' }
    & $armGxx -std=c++11 -Wall -Wextra -Werror -fsyntax-only `
        -I $apiInclude $cppFixture
    if ($LASTEXITCODE -ne 0) { throw 'ARM C++ public header fixture failed' }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'xStack phonebook API and four-slot control reserve behave correctly'
