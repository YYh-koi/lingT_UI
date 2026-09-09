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
    ('btstack_workers_' + [guid]::NewGuid().ToString('N'))
$exe = Join-Path $build 'test_btstack_workers.exe'

New-Item -ItemType Directory -Path $build | Out-Null
try {
    $includes = @(
        '-I', (Join-Path $root 'include\api'),
        '-I', (Join-Path $root 'include\adapter'),
        '-I', (Join-Path $root 'include'),
        '-I', (Join-Path $root 'api'),
        '-I', (Join-Path $root 'core'),
        '-I', (Join-Path $root 'src'),
        '-I', (Join-Path $root 'transport'))
    & $gcc -std=c99 -g -Wall -Wextra -Werror `
        -ffunction-sections -fdata-sections @includes `
        (Join-Path $root 'api\bt_xstack_api_core.c') `
        (Join-Path $core 'btstack_ring.c') `
        (Join-Path $core 'btstack_platform.c') `
        (Join-Path $core 'btstack_port_ops.c') `
        (Join-Path $tests 'test_btstack_workers.c') `
        '-Wl,--gc-sections' -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw 'Worker host test compile failed'
    }
    foreach ($scenario in @(
            'scheduling', 'reliable-space', 'partial-start', 'api-start',
            'enable-reentrant')) {
        Write-Output "worker scenario: $scenario"
        & $exe $scenario
        if ($LASTEXITCODE -ne 0) { throw "Worker scenario failed: $scenario" }
    }

    $apiText = Get-Content -Raw (Join-Path $root 'api\bt_xstack_api_core.c')
    if ($apiText -match '(?m)^\s*#\s*include\s*[<\"](?:FreeRTOS|task|queue)\.h[>\"]' -or
        $apiText -match '\b(?:xTask|vTask|ulTask|taskENTER|taskEXIT|TaskHandle_t|StaticTask_t|StackType_t|UBaseType_t|configMAX_PRIORITIES|tskIDLE_PRIORITY)\b') {
        throw 'Common API core still depends on FreeRTOS task APIs/types'
    }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'BTstack worker scheduling boundary behaves correctly'
