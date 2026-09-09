$ErrorActionPreference = 'Stop'

if (Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    $gcc = 'D:\mingw32\bin\gcc.exe'
}
else {
    $gcc = (Get-Command gcc.exe -ErrorAction Stop).Source
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$buildDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('btstack_adapter_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_adapter.exe'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I (Join-Path $root 'btstack/include') `
        -I (Join-Path $root 'btstack/src') `
        (Join-Path $root 'btstack/core/tests/test_btstack_adapter.c') `
        (Join-Path $root 'btstack/core/btstack_adapter.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'Adapter facade host test compile failed'
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'Adapter facade host test failed'
    }
    Write-Host 'BTstack adapter facade and neutral GAP events pass'
}
finally {
    Remove-Item -LiteralPath $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}
