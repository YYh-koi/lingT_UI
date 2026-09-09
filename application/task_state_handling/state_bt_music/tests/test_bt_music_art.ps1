$ErrorActionPreference = 'Stop'

$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$moduleDir = Split-Path -Parent $testDir
$repoRoot = (Resolve-Path (Join-Path $testDir '..\..\..\..')).Path
$gcc = 'D:\mingw32\bin\gcc.exe'
$output = Join-Path $env:TEMP 'test_bt_music_art.exe'

if (-not (Test-Path -LiteralPath $gcc)) {
    throw "MinGW GCC not found: $gcc"
}

$arguments = @(
    '-std=c99', '-Wall', '-Wextra', '-Werror',
    '-I', (Join-Path $testDir 'stubs'),
    '-I', (Join-Path $moduleDir 'inc'),
    (Join-Path $testDir 'test_bt_music_art.c'),
    (Join-Path $moduleDir 'src\bt_music_art.c'),
    '-o', $output
)

& $gcc @arguments
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $output
exit $LASTEXITCODE
