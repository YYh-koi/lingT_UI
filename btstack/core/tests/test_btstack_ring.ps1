param([string] $Compiler)

$ErrorActionPreference = 'Stop'

if ($Compiler) {
    $gcc = $Compiler
}
elseif (Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    $gcc = 'D:\mingw32\bin\gcc.exe'
}
else {
    $gcc = (Get-Command gcc.exe -ErrorAction Stop).Source
}

$testsDir = $PSScriptRoot
$coreDir = Split-Path -Parent $testsDir
$buildDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('btstack_ring_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_ring.exe'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I $coreDir `
        (Join-Path $coreDir 'btstack_ring.c') `
        (Join-Path $testsDir 'test_btstack_ring.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack ring test compilation failed: $LASTEXITCODE"
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack ring behavior test failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

Write-Output 'BTstack fixed ring preserves bounded FIFO behavior'
