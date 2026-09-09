param(
    [string]$ToolchainDirectory = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin',
    [string]$MakeProgram = 'D:\mingw32\bin\mingw32-make.exe',
    [int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'

$projectDirectory = $PSScriptRoot
$repositoryRoot = (Resolve-Path (Join-Path $projectDirectory '..\..\..')).Path
$projectFile = Join-Path $projectDirectory 'BTstack_AT_Library.gpcmprj'
$makeFile = Join-Path $projectDirectory 'BTstack_AT_Library_Debug.mk'
$generator = Join-Path $ToolchainDirectory 'gpcmprj2make.exe'
$generatorConfig = Join-Path $ToolchainDirectory 'gpcmprj2make.cfg'
$outputDirectory = Join-Path $repositoryRoot `
    'gplib\liba_wifi_src\atbm\atbm6165\bt_at'
$library = Join-Path $outputDirectory 'libxstack_at.a'
$publicHeaderDirectory = Join-Path $repositoryRoot `
    'btstack\include\at'

foreach ($required in @(
        $projectFile,
        $generator,
        $generatorConfig,
        $MakeProgram,
        $publicHeaderDirectory)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build input not found: $required"
    }
}
if ($Jobs -lt 1) {
    throw 'Jobs must be at least 1'
}

Push-Location $projectDirectory
try {
    & $generator -in $projectFile -cfg $generatorConfig -out $makeFile `
        -tp $ToolchainDirectory -windows -targets Debug --with-deps `
        --quote-path auto
    if ($LASTEXITCODE -ne 0) {
        throw "gpcmprj2make failed with exit code $LASTEXITCODE"
    }

    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
    $resolvedOutputDirectory = [IO.Path]::GetFullPath($outputDirectory)
    $resolvedLibrary = [IO.Path]::GetFullPath($library)
    if ([IO.Path]::GetDirectoryName($resolvedLibrary) -ne $resolvedOutputDirectory) {
        throw "Refusing to remove library outside output directory: $resolvedLibrary"
    }
    if (Test-Path -LiteralPath $library) {
        Remove-Item -LiteralPath $library -Force
    }

    $env:Path = "$ToolchainDirectory;$(Split-Path -Parent $MakeProgram);" + `
        $env:Path
    & $MakeProgram -B -f $makeFile "-j$Jobs"
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack AT build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
foreach ($header in @('bt_xstack.h', 'bt_xstack_cover_art.h')) {
    Copy-Item -LiteralPath (Join-Path $publicHeaderDirectory $header) `
        -Destination (Join-Path $outputDirectory $header) -Force
}

if (-not (Test-Path -LiteralPath $library)) {
    throw "Expected library was not produced: $library"
}

Write-Output "Built $library and packaged public headers"
