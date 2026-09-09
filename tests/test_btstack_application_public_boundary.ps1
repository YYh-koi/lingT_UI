$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$applicationRoots = @(
    (Join-Path $root 'application'),
    (Join-Path $root 'Project\wifi_demo'),
    (Join-Path $root 'Project\GPCV2178_Turnkey_SDK\src')
)
$projectPath = Join-Path $root `
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$compiler = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe'

$forbiddenIncludes = @()
foreach ($file in Get-ChildItem -LiteralPath $applicationRoots -Recurse -File `
        -Include *.c,*.h | Where-Object { $_.FullName -notmatch '[\\/]tests[\\/]' }) {
    foreach ($match in Select-String -LiteralPath $file.FullName `
            -Pattern '^\s*#\s*include\s+[<"]([^>"]+)[>"]') {
        $header = $match.Matches[0].Groups[1].Value
        if (($header -eq 'btstack_config.h') -or
            ($header -match '^(?:adapter|events|gap|profiles|core)/') -or
            ($header -match '^(?:btstack_|hci_|l2cap|avdtp|avrcp|hfp|pbap).*\.h$')) {
            $forbiddenIncludes += "${file}:$($match.LineNumber): $header"
        }
    }
}
if ($forbiddenIncludes.Count -ne 0) {
    throw "Application includes non-public BTstack headers:`n$($forbiddenIncludes -join "`n")"
}

[xml]$project = Get-Content -Raw -LiteralPath $projectPath
foreach ($target in $project.emIDE_project_file.Project.Build.Target) {
    foreach ($add in @($target.Compiler.Add | Where-Object { $_.directory })) {
        $directory = ([string]$add.directory).Replace('\', '/')
        if (($directory -match 'btstack') -and
            ($directory -notmatch '/atbm6165/(?:bt_api|bt_at)$')) {
            throw "Target $($target.title) exposes an internal BTstack include directory: $directory"
        }
    }
}

foreach ($packageName in @('bt_api', 'bt_at')) {
    $package = Join-Path $root `
        "gplib\liba_wifi_src\atbm\atbm6165\$packageName"
    $source = [IO.Path]::GetTempFileName()
    try {
        [IO.File]::WriteAllText(
            $source,
            "#include `"bt_xstack.h`"`nint main(void) { return 0; }`n",
            (New-Object Text.UTF8Encoding($false)))
        $output = @(& $compiler -std=c99 -Wall -Wextra -Werror `
            -fsyntax-only -x c "-I$package" $source 2>&1)
        if ($LASTEXITCODE -ne 0) {
            throw "Public $packageName header is not self-contained:`n$($output -join "`n")"
        }
    }
    finally {
        [IO.File]::Delete($source)
    }
}

Write-Output 'Application uses only self-contained packaged BTstack headers'
