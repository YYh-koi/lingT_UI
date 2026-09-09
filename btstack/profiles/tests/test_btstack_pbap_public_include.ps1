param([string] $Compiler = 'D:\mingw32\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$tests = $PSScriptRoot
$api = (Resolve-Path (Join-Path $tests '..\..')).Path
$build = Join-Path ([IO.Path]::GetTempPath()) ('pbap_public_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_public.exe'
New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $Compiler -std=c99 -Wall -Wextra -Werror -I (Join-Path $api 'include') `
        (Join-Path $tests 'test_btstack_pbap_public_include.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw "PBAP public include compilation failed: $LASTEXITCODE" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "PBAP public include executable failed: $LASTEXITCODE" }
    Write-Output 'PASS: PBAP public headers compile with only btstack_api/include'
} finally {
    if (Test-Path $build) { Remove-Item -LiteralPath $build -Recurse -Force }
}
