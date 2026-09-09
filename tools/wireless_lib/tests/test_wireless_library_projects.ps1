$ErrorActionPreference = 'Stop'
$script:Failures = 0

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

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

function Assert-SequenceEqual {
    param(
        [object[]]$Expected,
        [object[]]$Actual,
        [string]$Message
    )

    $expectedValues = @($Expected | ForEach-Object { [string]$_ })
    $actualValues = @($Actual | ForEach-Object { [string]$_ })
    if ($expectedValues.Count -ne $actualValues.Count) {
        $script:Failures++
        Write-Host "FAIL: $Message count (expected=$($expectedValues.Count) actual=$($actualValues.Count))" -ForegroundColor Red
        return
    }
    for ($index = 0; $index -lt $expectedValues.Count; $index++) {
        if ($expectedValues[$index] -cne $actualValues[$index]) {
            $script:Failures++
            Write-Host "FAIL: $Message at index $index" -ForegroundColor Red
            Write-Host "  expected: $($expectedValues[$index])" -ForegroundColor Red
            Write-Host "  actual:   $($actualValues[$index])" -ForegroundColor Red
            return
        }
    }
}

function Get-ProjectContract {
    param(
        [string]$ProjectPath
    )

    [xml]$project = Get-Content -LiteralPath $ProjectPath -Raw
    $projectDirectory = Split-Path -Parent $ProjectPath
    $targets = @($project.emIDE_project_file.Project.Build.Target)
    $target = $targets | Select-Object -First 1
    $typeOption = @($target.Option | Where-Object { $_.HasAttribute('type') }) | Select-Object -First 1
    $compilerOption = @($target.Option | Where-Object { $_.HasAttribute('compiler') }) | Select-Object -First 1
    $outputOption = @($target.Option | Where-Object { $_.HasAttribute('output') }) | Select-Object -First 1
    $options = @($target.Compiler.Add |
        Where-Object { $_.HasAttribute('option') } |
        ForEach-Object { [string]$_.option })
    $includes = @($target.Compiler.Add |
        Where-Object { $_.HasAttribute('directory') } |
        ForEach-Object {
            [IO.Path]::GetFullPath((Join-Path $projectDirectory ([string]$_.directory))).TrimEnd('\', '/')
        })
    $sources = @($project.emIDE_project_file.Project.Unit |
        Where-Object {
            $_.HasAttribute('filename') -and
            [IO.Path]::GetExtension([string]$_.filename).ToLowerInvariant() -eq '.c'
        } |
        ForEach-Object {
            [IO.Path]::GetFullPath((Join-Path $projectDirectory ([string]$_.filename)))
        })

    [PSCustomObject]@{
        Targets = $targets
        Target = $target
        Type = [string]$typeOption.type
        Compiler = [string]$compilerOption.compiler
        Output = [string]$outputOption.output
        PrefixAuto = [string]$outputOption.prefix_auto
        ExtensionAuto = [string]$outputOption.extension_auto
        TargetDevice = [string]$project.emIDE_project_file.Project.Extensions.projectwizard.TargetDevice
        Options = $options
        Includes = $includes
        Sources = $sources
    }
}

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolDirectory = Split-Path -Parent $testDirectory
$workspace = (Resolve-Path (Join-Path $toolDirectory '..\..')).Path
$modulePath = Join-Path $toolDirectory 'WirelessLibTools.psm1'
$manifestPath = Join-Path $toolDirectory 'wireless_sources.json'
$mainProjectPath = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$wifiProjectPath = Join-Path $workspace 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
$btProjectPath = Join-Path $workspace 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'

Import-Module $modulePath -Force
$manifest = Read-WirelessSourceManifest -Path $manifestPath -WorkspaceRoot $workspace
$mainConfiguration = Get-DebugCompilerConfiguration -ProjectFile $mainProjectPath
$rtkSdioRoot = [IO.Path]::GetFullPath((Join-Path $workspace `
    'gplib\liba_wifi_src\rtk\8189FTV\platform\sdio'))
$atbmSdioRoot = [IO.Path]::GetFullPath((Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\sdio'))
$wifiExpectedIncludes = @($mainConfiguration.IncludeDirectories |
    Where-Object {
        (-not $_.StartsWith($rtkSdioRoot, [StringComparison]::OrdinalIgnoreCase)) -and
        (-not $_.EndsWith('btstack\3rd-party\bluedroid\decoder\include', [StringComparison]::OrdinalIgnoreCase))
    })
$btExpectedIncludes = @($mainConfiguration.IncludeDirectories |
    Where-Object {
        -not $_.StartsWith($atbmSdioRoot, [StringComparison]::OrdinalIgnoreCase)
    })
$btExpectedIncludes += @(
    'btstack\ports\gpcv2178',
    'btstack\include',
    'btstack\src',
    'btstack\3rd-party\bluedroid\decoder\include',
    'btstack\3rd-party\md5',
    'btstack\3rd-party\yxml',
    'btstack\transport',
    'btstack\gap',
    'btstack\core',
    'btstack\at',
    'btstack\include\core',
    'btstack\include\at'
) | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $workspace $_)) } |
    Where-Object { $btExpectedIncludes -notcontains $_ }

Assert-True -Condition (Test-Path -LiteralPath $wifiProjectPath -PathType Leaf) -Message 'WiFi G+ project exists'
Assert-True -Condition (Test-Path -LiteralPath $btProjectPath -PathType Leaf) -Message 'BTstack G+ project exists'

if ((Test-Path -LiteralPath $wifiProjectPath -PathType Leaf) -and
    (Test-Path -LiteralPath $btProjectPath -PathType Leaf)) {
    $wifiProject = Get-ProjectContract -ProjectPath $wifiProjectPath
    $btProject = Get-ProjectContract -ProjectPath $btProjectPath

    foreach ($entry in @(
        [PSCustomObject]@{
            Name = 'WiFi'
            Project = $wifiProject
            ExpectedSources = @($manifest.Wifi | Sort-Object)
            ExpectedIncludes = $wifiExpectedIncludes
            ExpectedOutput = '../../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi'
        },
        [PSCustomObject]@{
            Name = 'BTstack'
            Project = $btProject
            ExpectedSources = @($manifest.Bluetooth | Sort-Object)
            ExpectedIncludes = $btExpectedIncludes
            ExpectedOutput = '../../../gplib/liba_wifi_src/atbm/atbm6165/bt/xstack'
        }
    )) {
        Assert-Equal -Expected 1 -Actual $entry.Project.Targets.Count -Message "$($entry.Name) has Debug target only"
        Assert-Equal -Expected 'Debug' -Actual ([string]$entry.Project.Target.title) -Message "$($entry.Name) target title"
        Assert-Equal -Expected '0' -Actual $entry.Project.Type -Message "$($entry.Name) static-library type"
        Assert-Equal -Expected 'armelfgcc' -Actual $entry.Project.Compiler -Message "$($entry.Name) toolchain"
        Assert-Equal -Expected $entry.ExpectedOutput -Actual $entry.Project.Output -Message "$($entry.Name) output"
        Assert-Equal -Expected '1' -Actual $entry.Project.PrefixAuto -Message "$($entry.Name) automatic prefix"
        Assert-Equal -Expected '1' -Actual $entry.Project.ExtensionAuto -Message "$($entry.Name) automatic extension"
        Assert-Equal -Expected 'GPA7740A' -Actual $entry.Project.TargetDevice -Message "$($entry.Name) G+ IC body"
        Assert-SequenceEqual `
            -Expected $entry.ExpectedSources `
            -Actual @($entry.Project.Sources | Sort-Object) `
            -Message "$($entry.Name) source membership"
        Assert-SequenceEqual `
            -Expected @($entry.ExpectedIncludes) `
            -Actual @($entry.Project.Includes) `
            -Message "$($entry.Name) include directories"
    }

    Assert-Equal -Expected 96 -Actual $wifiProject.Sources.Count -Message 'WiFi source count'
    Assert-Equal -Expected @($manifest.Bluetooth).Count -Actual $btProject.Sources.Count -Message 'BTstack source count'
    Assert-SequenceEqual `
        -Expected @($mainConfiguration.Options) `
        -Actual @($wifiProject.Options) `
        -Message 'WiFi compiler options'

    $btTargetConfigOption = '-include ../../../btstack/ports/gpcv2178/btstack_gpcv2178_config.h'
    $btBuildVariantOption = '-DBTSTACK_BUILD_VARIANT_AT=1'
    $btExpectedOptions = @($mainConfiguration.Options) + @(
        $btBuildVariantOption,
        $btTargetConfigOption
    )
    Assert-SequenceEqual `
        -Expected $btExpectedOptions `
        -Actual @($btProject.Options) `
        -Message 'BTstack compiler options'
}

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: dedicated wireless G+ project contract tests'
