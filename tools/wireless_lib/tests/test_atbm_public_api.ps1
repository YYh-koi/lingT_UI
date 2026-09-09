$ErrorActionPreference = 'Stop'
$script:Failures = 0

function Assert-Equal {
    param(
        $Expected,
        $Actual,
        [string]$Message
    )

    if ($Expected -ne $Actual) {
        $script:Failures++
        Write-Host "FAIL: $Message (expected=$Expected actual=$Actual)" -ForegroundColor Red
    }
}

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspace = (Resolve-Path (Join-Path $testDirectory '..\..\..')).Path
$apiDirectory = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\api'
$compiler = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe'
$fixtureRoot = Join-Path $workspace (
    'output\wireless_libs\test-atbm-public-api-' + [guid]::NewGuid().ToString('N')
)
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null

foreach ($header in @('atbm_wifi.h')) {
    $source = Join-Path $fixtureRoot ($header + '.c')
    Set-Content -LiteralPath $source -Value @(
        ('#include "{0}"' -f $header),
        'static atbm_wifi_connection_info_t g_connection_info;',
        'static atbm_wifi_ip_info_t g_ip_info;',
        'static atbm_wifi_client_info_t g_client_info;',
        'int atbm_public_header_probe(void)',
        '{',
        '    return (int)sizeof(g_connection_info) +',
        '        (int)sizeof(g_ip_info) +',
        '        (int)sizeof(g_client_info);',
        '}'
    )
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & $compiler `
        '-fsyntax-only' `
        '-std=gnu99' `
        '-mcpu=cortex-a7' `
        '-mfloat-abi=softfp' `
        "-I$apiDirectory" `
        $source 2>&1
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference
    if ($exitCode -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
    }
    Assert-Equal -Expected 0 -Actual $exitCode -Message "$header is self-contained"
}

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: ATBM public API headers compile without private include paths'
