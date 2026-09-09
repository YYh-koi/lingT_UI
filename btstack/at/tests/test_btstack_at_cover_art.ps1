param([string] $Compiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (Test-Path 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$at = Split-Path -Parent $tests
$root = Split-Path -Parent $at
$temp = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack_at_cover_art_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $temp 'test_btstack_at_cover_art.exe'
New-Item -ItemType Directory -Path $temp | Out-Null
try {
    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -I (Join-Path $root 'include') `
        -I (Join-Path $root 'include\adapter') `
        -I $at -I (Join-Path $root 'core') `
        -I (Join-Path $root 'src') `
        -I (Join-Path $root 'transport') `
        (Join-Path $at 'bt_xstack_at_frontend.c') `
        (Join-Path $tests 'test_btstack_at_cover_art.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "AT Cover Art frontend compile failed: $LASTEXITCODE"
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "AT Cover Art frontend behavior failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path $temp) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }
}
Write-Output 'AT Cover Art callback is direct, transient, and outside critical sections'
