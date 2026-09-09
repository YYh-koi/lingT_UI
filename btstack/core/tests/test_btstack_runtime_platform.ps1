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
    ('btstack_runtime_platform_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_runtime_platform.exe'
$mutantExe = Join-Path $build 'test_btstack_runtime_platform_mutant.exe'
$mutantSource = Join-Path $build 'btstack_runtime_mutant.c'
$mutantOut = Join-Path $build 'mutant.out.txt'
$mutantErr = Join-Path $build 'mutant.err.txt'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $includes = @(
        '-I', (Join-Path $root 'include'),
        '-I', (Join-Path $root 'include\adapter'),
        '-I', (Join-Path $root 'src'),
        '-I', (Join-Path $root 'transport'),
        '-I', (Join-Path $root 'core'),
        '-I', (Join-Path $root 'ports\gpcv2178'),
        '-I', (Join-Path $root 'profiles'))
    & $gcc -std=c99 -Wall -Wextra -Werror -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -ffunction-sections -fdata-sections @includes `
        (Join-Path $core 'btstack_runtime.c') `
        (Join-Path $core 'btstack_platform.c') `
        (Join-Path $core 'btstack_port_ops.c') `
        (Join-Path $core 'btstack_ring.c') `
        (Join-Path $root 'api\bt_xstack_api_core.c') `
        (Join-Path $root 'ports\gpcv2178\btstack_gpcv2178_port.c') `
        (Join-Path $tests 'test_btstack_runtime_platform.c') `
        '-Wl,--gc-sections' -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'Runtime platform host test compile failed'
    }
    foreach ($scenario in @(
            'success', 'early-fail', 'after-hci-fail', 'adapter-fail',
            'profile-manager-fail', 'backend-fail', 'stack-start-fail',
            'reentrant-start', 'reentrant-init', 'legacy-to-api',
            'api-to-legacy', 'diagnostics-deinit')) {
        & $exe $scenario
        if ($LASTEXITCODE -ne 0) {
            throw "Runtime platform scenario failed: $scenario"
        }
    }

    $runtimeText = Get-Content -Raw (Join-Path $core 'btstack_runtime.c')
    if ($runtimeText -match '(?i)gpcv2178') {
        throw 'Common runtime still contains a GPCV2178 identifier'
    }
    $stackOpsText = Get-Content -Raw (Join-Path $root `
        'include\port\btstack_stack_ops.h')
    if ($stackOpsText -notmatch
        '(?s)bounded and non-blocking.*system critical section is held.*get_diagnostics') {
        throw 'Stack diagnostics ABI is missing its critical-section contract'
    }

    $mutationPattern = '(?s)(memset\(&stack_diagnostics, 0, sizeof\(stack_diagnostics\)\);\s*)(platform->stack\.get_diagnostics\(platform->stack\.context,\s*&stack_diagnostics\);)'
    $mutationReplacement = "`$1btstack_platform_exit_critical();`r`n    `$2`r`n    btstack_platform_enter_critical();"
    $mutantText = [regex]::Replace($runtimeText, $mutationPattern,
        $mutationReplacement, 1)
    if ($mutantText -eq $runtimeText) {
        throw 'Runtime diagnostics mutation insertion point was not found'
    }
    Set-Content -LiteralPath $mutantSource -Encoding Ascii -Value $mutantText
    & $gcc -std=c99 -Wall -Wextra -Werror -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -ffunction-sections -fdata-sections @includes `
        $mutantSource `
        (Join-Path $core 'btstack_platform.c') `
        (Join-Path $core 'btstack_port_ops.c') `
        (Join-Path $core 'btstack_ring.c') `
        (Join-Path $root 'api\bt_xstack_api_core.c') `
        (Join-Path $root 'ports\gpcv2178\btstack_gpcv2178_port.c') `
        (Join-Path $tests 'test_btstack_runtime_platform.c') `
        '-Wl,--gc-sections' -o $mutantExe
    if ($LASTEXITCODE -ne 0) {
        throw 'Runtime diagnostics mutant compile failed'
    }
    $mutant = Start-Process -FilePath $mutantExe `
        -ArgumentList 'diagnostics-deinit' -Wait -PassThru `
        -WindowStyle Hidden -RedirectStandardOutput $mutantOut `
        -RedirectStandardError $mutantErr
    if ($mutant.ExitCode -eq 0) {
        throw 'Diagnostics lock-removal mutation survived the interleaving test'
    }
}
finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'BTstack runtime platform lifecycle behaves correctly'
