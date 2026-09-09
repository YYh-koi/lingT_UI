param(
    [string] $Compiler
)

$ErrorActionPreference = 'Stop'

if ($Compiler) {
    $gcc = $Compiler
}
elseif (Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    $gcc = 'D:\mingw32\bin\gcc.exe'
}
else {
    $gccCommand = Get-Command gcc.exe -ErrorAction Stop
    $gcc = $gccCommand.Source
}
$testsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$profilesDir = Split-Path -Parent $testsDir
$apiDir = Split-Path -Parent $profilesDir
$buildDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('btstack_vcard_parser_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_vcard_parser.exe'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    & $gcc `
        -std=c99 -O0 -Wall -Wextra -Werror `
        -I (Join-Path $apiDir 'include') `
        (Join-Path $profilesDir 'btstack_vcard_parser.c') `
        (Join-Path $testsDir 'test_btstack_vcard_parser.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "vCard parser test compilation failed: $LASTEXITCODE"
    }

    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "vCard parser tests failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}
