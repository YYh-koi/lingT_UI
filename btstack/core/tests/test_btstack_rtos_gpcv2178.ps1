param([string] $Compiler)

$ErrorActionPreference = 'Stop'
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$tests = $PSScriptRoot
$core = Split-Path -Parent $tests
$root = Split-Path -Parent $core
$build = Join-Path ([IO.Path]::GetTempPath()) `
    ('btstack_rtos_gpcv2178_' + [guid]::NewGuid().ToString('N'))
$apiExe = Join-Path $build 'test_btstack_rtos_gpcv2178_api.exe'
$atExe = Join-Path $build 'test_btstack_rtos_gpcv2178_at.exe'
$mutantSource = Join-Path $build 'btstack_rtos_gpcv2178_mutant.c'
$mutantExe = Join-Path $build 'test_btstack_rtos_gpcv2178_mutant.exe'
$mutantStdout = Join-Path $build 'mutant.stdout.txt'
$mutantStderr = Join-Path $build 'mutant.stderr.txt'
$portConfig = Join-Path $root 'ports\gpcv2178\btstack_gpcv2178_config.h'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    foreach ($variant in @(
            @('BTSTACK_BUILD_VARIANT_API', $apiExe),
            @('BTSTACK_BUILD_VARIANT_AT', $atExe))) {
        & $gcc -std=c99 -Wall -Wextra -Werror `
            "-D$($variant[0])=1" `
            -I (Join-Path $tests 'rtos_gpcv2178_stubs') `
            -I (Join-Path $root 'ports\gpcv2178') `
            -I (Join-Path $root 'include') `
            -include $portConfig `
            (Join-Path $root 'ports\gpcv2178\btstack_rtos_gpcv2178.c') `
            (Join-Path $tests 'test_btstack_rtos_gpcv2178.c') `
            -o $variant[1]
        if ($LASTEXITCODE -ne 0) {
            throw "GPCV2178 RTOS $($variant[0]) compile failed"
        }
        & $variant[1]
        if ($LASTEXITCODE -ne 0) {
            throw "GPCV2178 RTOS $($variant[0]) behavior failed"
        }
    }

    $productionSource = Join-Path $root `
        'ports\gpcv2178\btstack_rtos_gpcv2178.c'
    $sourceText = Get-Content -Raw -LiteralPath $productionSource
    $mutantText = $sourceText.Replace(
        '    notify_result = xTaskNotify(handle, signals, eSetBits);',
        "    taskEXIT_CRITICAL();`r`n" +
        '    notify_result = xTaskNotify(handle, signals, eSetBits);' +
        "`r`n    taskENTER_CRITICAL();")
    if ($mutantText -eq $sourceText) {
        throw 'Notify snapshot-unlock mutation insertion point was not found'
    }
    Set-Content -LiteralPath $mutantSource -Encoding Ascii -Value $mutantText
    & $gcc -std=c99 -Wall -Wextra -Werror `
        -DBTSTACK_BUILD_VARIANT_API=1 `
        -I (Join-Path $tests 'rtos_gpcv2178_stubs') `
        -I (Join-Path $root 'ports\gpcv2178') `
        -I (Join-Path $root 'include') `
        -include $portConfig `
        $mutantSource `
        (Join-Path $tests 'test_btstack_rtos_gpcv2178.c') `
        -o $mutantExe
    if ($LASTEXITCODE -ne 0) { throw 'GPCV2178 notify mutant compile failed' }
    $mutant = Start-Process -FilePath $mutantExe -Wait -PassThru `
        -WindowStyle Hidden -RedirectStandardOutput $mutantStdout `
        -RedirectStandardError $mutantStderr
    if ($mutant.ExitCode -eq 0) {
        throw 'Snapshot-unlock notify mutation survived lifecycle test'
    }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'GPCV2178 RTOS worker adapter state and signals behave correctly'
