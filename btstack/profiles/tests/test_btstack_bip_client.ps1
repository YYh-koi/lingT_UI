param([string] $Compiler = 'D:\mingw32\bin\gcc.exe', [switch] $ParserOnly)

$ErrorActionPreference = 'Stop'
$tests = $PSScriptRoot
$root = (Resolve-Path (Join-Path $tests '..\..')).Path
$build = Join-Path ([IO.Path]::GetTempPath()) ('btstack_bip_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_bip.exe'
$object = Join-Path $build 'btstack_bip_client.o'
$nm = Join-Path (Split-Path -Parent $Compiler) 'nm.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $Compiler -std=c99 -O0 -Wall -Wextra -Werror -DBTSTACK_BIP_CLIENT_TESTING=1 `
        -I (Join-Path $root 'include') -I (Join-Path $root 'include\core') `
        -I (Join-Path $root 'src') -I (Join-Path $root 'transport') -I (Join-Path $root '3rd-party\yxml') `
        (Join-Path $root 'profiles\btstack_bip_client.c') `
        (Join-Path $root '3rd-party\yxml\yxml.c') `
        (Join-Path $root 'src\btstack_util.c') `
        (Join-Path $tests 'test_btstack_bip_client.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'BIP client compilation failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'BIP client tests failed' }
    & $Compiler -std=c99 -O0 -Wall -Wextra -Werror -DBTSTACK_BIP_CLIENT_TESTING=1 `
        -I (Join-Path $root 'include') -I (Join-Path $root 'include\core') `
        -I (Join-Path $root 'src') -I (Join-Path $root 'transport') -I (Join-Path $root '3rd-party\yxml') -c `
        (Join-Path $root 'profiles\btstack_bip_client.c') -o $object
    if ($LASTEXITCODE -ne 0) { throw 'BIP client object compilation failed' }
    $undefined = (& $nm -u $object | Out-String)
    if ($undefined -match '(?i)\b(malloc|calloc|realloc|free)\b') {
        throw 'BIP client parser must not use heap allocation'
    }
} finally {
    if (Test-Path -LiteralPath $build) { Remove-Item -LiteralPath $build -Recurse -Force }
}
