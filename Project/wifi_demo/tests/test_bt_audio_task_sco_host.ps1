$ErrorActionPreference = 'Stop'

$testRoot = $PSScriptRoot
$compiler = 'D:\mingw32\bin\gcc.exe'
$source = Join-Path $testRoot 'bt_audio_task_sco_host_test.c'
$stubRoot = Join-Path $testRoot 'host_stubs'
$includeRoot = [IO.Path]::GetFullPath((Join-Path $testRoot '..\..\Include'))
$buildRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("bt_audio_sco_host_{0}" -f $PID)
$executable = Join-Path $buildRoot 'bt_audio_task_sco_host_test.exe'

foreach ($required in @($compiler, $source, $stubRoot, $includeRoot)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing host-test input: $required"
    }
}

try {
    New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
    & $compiler -std=c99 -O0 -Wall -Wextra `
        "-I$stubRoot" "-I$includeRoot" $source -o $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Host C compilation failed with exit code $LASTEXITCODE"
    }
    & $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Host SCO state-machine test failed with exit code $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $buildRoot) {
        $resolvedBuildRoot = [IO.Path]::GetFullPath($buildRoot)
        $resolvedTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
        if ($resolvedBuildRoot.StartsWith(
                $resolvedTempRoot,
                [StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $resolvedBuildRoot -Recurse -Force
        }
    }
}
