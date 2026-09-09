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
    ('btstack_at_stack_ingress_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_at_stack_ingress_retry.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -DBTSTACK_BUILD_VARIANT_AT=1 `
        -I (Join-Path $tests 'run_loop_gpcv2178_stubs') `
        -I (Join-Path $root 'ports\gpcv2178') `
        -I (Join-Path $root 'include') `
        -I (Join-Path $root 'include\adapter') `
        -I (Join-Path $root 'src') `
        -I (Join-Path $root 'core') `
        -I (Join-Path $root 'transport') `
        -I $at `
        (Join-Path $at 'btstack_at_core_adapter.c') `
        (Join-Path $tests 'test_btstack_at_stack_ingress_retry.c') `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "AT stack-ingress retry test compilation failed: $LASTEXITCODE"
    }
    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "AT stack-ingress retry behavior failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'AT LOST cleanup retries when real GPC run-loop dequeue frees ingress'
