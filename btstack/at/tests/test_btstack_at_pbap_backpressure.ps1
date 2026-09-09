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
$atDir = Split-Path -Parent $testsDir
$btstackRoot = Split-Path -Parent $atDir
$buildDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('btstack_at_pbap_backpressure_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_at_pbap_backpressure.exe'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -I (Join-Path $btstackRoot 'include') `
        -I (Join-Path $btstackRoot 'include\adapter') `
        -I $atDir `
        -I (Join-Path $btstackRoot 'core') `
        -I (Join-Path $btstackRoot 'src') `
        -I (Join-Path $btstackRoot 'transport') `
        (Join-Path $atDir 'btstack_at_core_adapter.c') `
        (Join-Path $atDir 'btstack_at_protocol.c') `
        (Join-Path $testsDir 'test_btstack_at_pbap_backpressure.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack AT PBAP backpressure test compilation failed: $LASTEXITCODE"
    }

    foreach ($scenario in @(
            'accepted', 'busy', 'hfp-lost', 'hfp-lost-retry',
            'hfp-lost-idle-pbap', 'hfp-lost-three-serialized')) {
        & $exe $scenario
        if ($LASTEXITCODE -ne 0) {
            throw "BTstack AT PBAP backpressure scenario '$scenario' failed: $LASTEXITCODE"
        }
    }
}
finally {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

Write-Output 'AT PBAP cleanup is ordered, serialized, and retried by stack ingress space'
