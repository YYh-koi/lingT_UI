$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gcc = 'D:\mingw32\bin\gcc.exe'
$build = Join-Path ([System.IO.Path]::GetTempPath()) ('btstack_gatt_server_' + [Guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_gatt_server.exe'
New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I (Join-Path $root 'btstack/include') -I (Join-Path $root 'btstack/src') `
        -I (Join-Path $root 'btstack/transport') `
        (Join-Path $root 'btstack/profiles/tests/test_btstack_gatt_server.c') `
        (Join-Path $root 'btstack/profiles/btstack_gatt_server.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'GATT Server host fixture compile failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'GATT Server host fixture failed' }
    Write-Host 'BTstack bounded GATT Server registration, freeze, value pool, and TX PASS'
}
finally { Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction SilentlyContinue }
