$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gcc = 'D:\mingw32\bin\gcc.exe'
$build = Join-Path ([System.IO.Path]::GetTempPath()) ('btstack_gatt_client_' + [Guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_gatt_client.exe'
New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I (Join-Path $root 'btstack/include') -I (Join-Path $root 'btstack/src') `
        -I (Join-Path $root 'btstack/transport') `
        (Join-Path $root 'btstack/profiles/tests/test_btstack_gatt_client.c') `
        (Join-Path $root 'btstack/profiles/btstack_gatt_client.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'GATT Client host fixture compile failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'GATT Client host fixture failed' }
    Write-Host 'BTstack serialized GATT Client operations and disconnect cleanup PASS'
}
finally { Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction SilentlyContinue }
