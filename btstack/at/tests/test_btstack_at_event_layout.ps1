param([string] $Compiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$at = Split-Path -Parent $tests
$root = Split-Path -Parent $at
$build = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack_at_event_layout_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_at_event_layout.exe'
$armObject = Join-Path $build 'event_size.o'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -I (Join-Path $root 'include') `
        -I (Join-Path $root 'include\adapter') `
        -I (Join-Path $root 'core') `
        -I (Join-Path $root 'src') `
        -I (Join-Path $root 'transport') `
        -I $at `
        (Join-Path $at 'btstack_at_protocol.c') `
        (Join-Path $tests 'test_btstack_at_protocol.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'AT event layout test compilation failed'
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'AT event layout/format behavior failed'
    }

    $armGcc = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe'
    $armNm = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe'
    if ((Test-Path -LiteralPath $armGcc) -and
        (Test-Path -LiteralPath $armNm)) {
        "#include `"btstack_at_internal.h`"`n" +
            'char btstack_at_event_size[sizeof(btstack_at_event_t)];' |
            & $armGcc -x c -std=c99 `
                -I (Join-Path $root 'include') `
                -I (Join-Path $root 'include\adapter') `
                -I (Join-Path $root 'core') `
                -I (Join-Path $root 'src') `
                -I (Join-Path $root 'transport') `
                -I $at -c -o $armObject -
        if ($LASTEXITCODE -ne 0) {
            throw 'AT event ARM size probe compilation failed'
        }
        $symbols = & $armNm -S $armObject
        if ($LASTEXITCODE -ne 0 -or
            $symbols -notmatch '(?im)^\S+\s+0000010c\s+[A-Za-z]\s+btstack_at_event_size$') {
            throw 'AT event ARM size must be exactly 268 bytes with bounded GATT payload'
        }
    }
}
finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'AT event tagged union is compact and wire-compatible'
