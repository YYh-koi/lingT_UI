param([string] $Compiler = 'D:\mingw32\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$tests = $PSScriptRoot
$api = (Resolve-Path (Join-Path $tests '..\..')).Path
$build = Join-Path ([IO.Path]::GetTempPath()) ('pbap_profile_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_pbap.exe'
New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $Compiler -std=c99 -O0 -Wall -Wextra -Werror `
        -I (Join-Path $tests 'pbap_host_stubs') -I (Join-Path $api 'include') -I (Join-Path $api 'profiles') `
        (Join-Path $api 'profiles\btstack_pbap_client.c') `
        (Join-Path $api 'profiles\btstack_vcard_parser.c') `
        (Join-Path $tests 'test_btstack_pbap_state_machine.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw "PBAP state-machine compilation failed: $LASTEXITCODE" }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "PBAP state-machine tests failed: $LASTEXITCODE" }
} finally {
    if (Test-Path $build) { Remove-Item -LiteralPath $build -Recurse -Force }
}
