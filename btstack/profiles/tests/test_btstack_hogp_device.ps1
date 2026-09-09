$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$gcc = 'D:\mingw32\bin\gcc.exe'
if (-not (Test-Path $gcc)) { throw "Missing compiler: $gcc" }
$out = Join-Path $env:TEMP 'test_btstack_hogp_device.exe'
& $gcc -std=c99 -Wall -Wextra -Werror `
    -DBTSTACK_CLASSIC=0 -DBTSTACK_LE=1 -DBTSTACK_GATT_SERVER=1 -DBTSTACK_HOGP_DEVICE=1 `
    -I (Join-Path $root 'include') -I (Join-Path $root 'src') -I $root `
    (Join-Path $root 'profiles\btstack_hogp_device.c') `
    (Join-Path $PSScriptRoot 'test_btstack_hogp_device.c') -o $out
if ($LASTEXITCODE -ne 0) { throw 'HOGP device compile failed' }
& $out
if ($LASTEXITCODE -ne 0) { throw 'HOGP device test failed' }
Write-Host 'HOGP device test PASS'
