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
    ('btstack_at_parser_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_at_parser.exe'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -I (Join-Path $btstackRoot 'include') `
        -I (Join-Path $btstackRoot 'include\adapter') `
        -I $atDir `
        -I (Join-Path $btstackRoot 'core') `
        -I (Join-Path $btstackRoot 'src') `
        -I (Join-Path $btstackRoot 'transport') `
        (Join-Path $atDir 'btstack_at_parser.c') `
        (Join-Path $atDir 'btstack_at_command_table.c') `
        (Join-Path $testsDir 'test_btstack_at_parser.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack AT parser test compilation failed: $LASTEXITCODE"
    }

    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack AT parser behavior test failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

Write-Output 'BTstack AT parser accepts bounded LE and GATT commands'
