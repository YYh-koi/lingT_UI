$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$gcc = 'D:\mingw32\bin\gcc.exe'
$build = Join-Path ([System.IO.Path]::GetTempPath()) ('btstack_gap_le_' + [Guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_gap_le.exe'
New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I (Join-Path $root 'btstack/include') `
        -I (Join-Path $root 'btstack/src') `
        -I (Join-Path $root 'btstack/transport') `
        -I (Join-Path $root 'btstack/gap') `
        (Join-Path $root 'btstack/gap/tests/test_btstack_gap_le.c') `
        (Join-Path $root 'btstack/gap/btstack_gap_le.c') -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'LE GAP host fixture compile failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'LE GAP host fixture failed' }
    Write-Host 'BTstack Legacy LE GAP advertising, scanning, and one-link state PASS'
}
finally {
    Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction SilentlyContinue
}
