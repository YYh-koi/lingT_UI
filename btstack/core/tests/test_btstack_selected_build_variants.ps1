param(
    [string]$Compiler,
    [string]$Nm
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$gcc = if ($Compiler) { $Compiler } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    'D:\mingw32\bin\gcc.exe'
} else { (Get-Command gcc.exe -ErrorAction Stop).Source }
$nmTool = if ($Nm) { $Nm } elseif (
    Test-Path -LiteralPath 'D:\mingw32\bin\nm.exe') {
    'D:\mingw32\bin\nm.exe'
} else { (Get-Command nm.exe -ErrorAction Stop).Source }
$objdumpTool = Join-Path (Split-Path -Parent $nmTool) 'objdump.exe'
if (-not (Test-Path -LiteralPath $objdumpTool)) {
    $objdumpTool = (Get-Command objdump.exe -ErrorAction Stop).Source
}

$apiDefine = 'BTSTACK_BUILD_VARIANT_API'
$atDefine = 'BTSTACK_BUILD_VARIANT_AT'
$projectSpecs = @(
    @{
        Name = 'API'
        Path = 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj'
        Define = $apiDefine
        ForbiddenDefine = $atDefine
        Frontend = @('btstack/api/bt_xstack_api_core.c')
        SelectedProfiles = @(
            'btstack/src/classic/avrcp_cover_art_client.c',
            'btstack/profiles/btstack_bip_client.c',
            'btstack/profiles/btstack_avrcp_cover_art.c'
        )
    },
    @{
        Name = 'AT'
        Path = 'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library.gpcmprj'
        Define = $atDefine
        ForbiddenDefine = $apiDefine
        Frontend = @(
            'btstack/at/btstack_at.c',
            'btstack/at/btstack_at_command_table.c',
            'btstack/at/btstack_at_dispatch_table.c',
            'btstack/at/btstack_at_parser.c',
            'btstack/at/btstack_at_protocol.c',
            'btstack/at/btstack_at_core_adapter.c',
            'btstack/at/bt_xstack_at_frontend.c'
        )
        SelectedProfiles = @(
            'btstack/src/classic/avrcp_cover_art_client.c',
            'btstack/profiles/btstack_bip_client.c',
            'btstack/profiles/btstack_avrcp_cover_art.c'
        )
    },
    @{
        Name = 'Legacy AT'
        Path = 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'
        Define = $atDefine
        ForbiddenDefine = $apiDefine
        Frontend = @(
            'btstack/at/btstack_at.c',
            'btstack/at/btstack_at_command_table.c',
            'btstack/at/btstack_at_dispatch_table.c',
            'btstack/at/btstack_at_parser.c',
            'btstack/at/btstack_at_protocol.c',
            'btstack/at/btstack_at_core_adapter.c'
        )
        SelectedProfiles = @()
    }
)

function Get-ProjectSources([string]$projectPath) {
    [xml]$project = Get-Content -Raw -LiteralPath $projectPath
    $projectDirectory = Split-Path -Parent $projectPath
    $rootPrefix = $repoRoot.TrimEnd('\') + '\'
    return @($project.SelectNodes('//Unit') | ForEach-Object {
        $absolute = [IO.Path]::GetFullPath((
            Join-Path $projectDirectory ([string]$_.filename)))
        if (-not $absolute.StartsWith(
                $rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Project source escapes repository root: $absolute"
        }
        $absolute.Substring($rootPrefix.Length).Replace('\', '/')
    })
}

function Assert-ExactSet([string]$name, [string[]]$actual, [string[]]$expected) {
    $duplicates = @($actual | Group-Object | Where-Object Count -ne 1)
    if ($duplicates.Count -ne 0) {
        throw "$name has duplicate sources: $($duplicates.Name -join ', ')"
    }
    $missing = @($expected | Where-Object { $_ -notin $actual })
    $extra = @($actual | Where-Object { $_ -notin $expected })
    if (($missing.Count -ne 0) -or ($extra.Count -ne 0)) {
        throw "$name source set mismatch; missing=[$($missing -join ', ')] extra=[$($extra -join ', ')]"
    }
}

function Assert-SelectedDefine([string]$name, [string]$text,
    [string]$required, [string]$forbidden) {
    $requiredCount = [regex]::Matches(
        $text, '-D' + [regex]::Escape($required) + '(?:=1)?(?=["\s<])').Count
    $forbiddenCount = [regex]::Matches(
        $text, '-D' + [regex]::Escape($forbidden) + '(?:=1)?(?=["\s<])').Count
    if ($requiredCount -ne 1) {
        throw "$name must define $required exactly once (actual=$requiredCount)"
    }
    if ($forbiddenCount -ne 0) {
        throw "$name must not define $forbidden (actual=$forbiddenCount)"
    }
}

function Get-ObjectSectionSize([string]$sections, [string]$symbol) {
    $match = [regex]::Match($sections,
        '(?m)^\s*\d+\s+\.(?:data|bss)\$' + [regex]::Escape($symbol) +
        '\s+([0-9a-fA-F]+)\s+')
    if (-not $match.Success) {
        throw "Object section for $symbol is missing"
    }
    return [Convert]::ToUInt32($match.Groups[1].Value, 16)
}

# Mutation proof: a project without the selected define must be rejected.
$mutationCaught = $false
try {
    Assert-SelectedDefine 'mutant API' '<Compiler />' $apiDefine $atDefine
} catch {
    $mutationCaught = $true
}
if (-not $mutationCaught) {
    throw 'Selected-build define mutation survived the contract helper'
}

$manifest = Get-Content -Raw -LiteralPath (
    Join-Path $repoRoot 'tools\wireless_lib\wireless_sources.json') |
    ConvertFrom-Json
$makeManifest = Get-Content -Raw -LiteralPath (
    Join-Path $repoRoot 'btstack\Makefile.inc')
if ($makeManifest -notmatch '(?m)^BTSTACK_API_BUILD_CPPFLAGS\s*:=\s*-DBTSTACK_BUILD_VARIANT_API=1\s*$') {
    throw 'Common Make manifest does not expose the API selected-build flag'
}
if ($makeManifest -notmatch '(?m)^BTSTACK_AT_BUILD_CPPFLAGS\s*:=\s*-DBTSTACK_BUILD_VARIANT_AT=1\s*$') {
    throw 'Common Make manifest does not expose the AT selected-build flag'
}
if ($makeManifest -match 'BTSTACK_(?:API|AT)_FRONTEND_CPPFLAGS') {
    throw 'Frontend-scoped CPPFLAGS names must not describe target-wide build selection'
}
$makeProgram = if (Test-Path -LiteralPath 'D:\mingw32\bin\mingw32-make.exe') {
    'D:\mingw32\bin\mingw32-make.exe'
} else {
    (Get-Command make.exe -ErrorAction Stop).Source
}
$makeIncludePath = (Join-Path $repoRoot 'btstack\Makefile.inc').Replace('\', '/')
$makeFixture = @'
include __BTSTACK_MAKEFILE__
.PHONY: print-selected-flags
print-selected-flags:
	@echo API=$(BTSTACK_API_BUILD_CPPFLAGS)
	@echo AT=$(BTSTACK_AT_BUILD_CPPFLAGS)
'@.Replace('__BTSTACK_MAKEFILE__', $makeIncludePath)
$makeOutput = @($makeFixture | & $makeProgram -s -f - print-selected-flags)
if ($LASTEXITCODE -ne 0) {
    throw 'Common Make selected-build variables cannot be evaluated'
}
if (($makeOutput -notcontains 'API=-DBTSTACK_BUILD_VARIANT_API=1') -or
    ($makeOutput -notcontains 'AT=-DBTSTACK_BUILD_VARIANT_AT=1')) {
    throw "Common Make selected-build behavior is wrong: $($makeOutput -join '; ')"
}
$canonical = @($manifest.bluetooth)
$portSources = @($canonical | Where-Object {
        $_ -like 'btstack/ports/gpcv2178/*'
    })
$atCoreSources = @($canonical | Where-Object {
        $_ -like 'btstack/at/*'
    })
$commonSources = @($canonical | Where-Object {
        ($_ -notin $portSources) -and ($_ -notin $atCoreSources)
    })
if (($portSources.Count -ne 8) -or ($atCoreSources.Count -ne 6)) {
    throw "Canonical manifest partition is stale: port=$($portSources.Count), at=$($atCoreSources.Count)"
}

foreach ($spec in $projectSpecs) {
    $projectPath = Join-Path $repoRoot $spec.Path
    $projectText = Get-Content -Raw -LiteralPath $projectPath
    Assert-SelectedDefine $spec.Name $projectText `
        $spec.Define $spec.ForbiddenDefine
    $actual = @(Get-ProjectSources $projectPath)
    $expected = @($commonSources + $portSources + $spec.Frontend +
        $spec.SelectedProfiles)
    Assert-ExactSet $spec.Name $actual $expected

    $portOwnership = @($actual | Where-Object {
            $_ -eq 'btstack/ports/gpcv2178/btstack_rtos_gpcv2178.c'
        })
    if ($portOwnership.Count -ne 1) {
        throw "$($spec.Name) must own exactly one GPCV2178 worker port"
    }
    if (($spec.Name -eq 'API') -and
        (@($actual | Where-Object { $_ -like 'btstack/at/*' }).Count -ne 0)) {
        throw 'API project includes AT frontend sources'
    }
    if (($spec.Name -ne 'API') -and
        (@($actual | Where-Object { $_ -like 'btstack/api/*' }).Count -ne 0)) {
        throw "$($spec.Name) project includes API frontend sources"
    }
}

$readme = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'btstack\README.md')
$portsReadme = Get-Content -Raw -LiteralPath (
    Join-Path $repoRoot 'btstack\ports\README.md')
$documentationChecks = @(
    [pscustomobject]@{ Name = 'AT lifecycle'; Pass = $readme -match '(?s)AT worker.*AT\+BTEN=1.*STACK worker.*API/EVENT' },
    [pscustomobject]@{ Name = 'AT ring capacities'; Pass = $readme -match '(?s)Input.*8.*Completion.*1.*Event.*16' },
    [pscustomobject]@{ Name = 'AT worker signals'; Pass = $readme -match '(?s)WORK.*COMPLETION' },
    [pscustomobject]@{ Name = 'AT total timeout'; Pass = $readme -match '(?s)1000\s*ms.*total' },
    [pscustomobject]@{ Name = 'AT overflow policy'; Pass = $readme -match '(?s)overflow.*drop|drop.*overflow' },
    [pscustomobject]@{ Name = 'selected build defines'; Pass = $portsReadme -match $apiDefine -and $portsReadme -match $atDefine },
    [pscustomobject]@{ Name = 'selected static resources'; Pass = $portsReadme -match '(?s)API.*API.*EVENT.*stack.*AT.*AT.*stack' },
    [pscustomobject]@{ Name = 'selected task controls'; Pass = $portsReadme -match '(?s)API.*API.*EVENT.*task controls.*AT.*one AT task control' },
    [pscustomobject]@{ Name = 'API build flags consumed'; Pass = $readme -match '(?m)^API_LIBRARY_CPPFLAGS\s*:=\s*\$\(BTSTACK_API_BUILD_CPPFLAGS\)\s*$' },
    [pscustomobject]@{ Name = 'AT build flags consumed'; Pass = $readme -match '(?m)^AT_LIBRARY_CPPFLAGS\s*:=\s*\$\(BTSTACK_AT_BUILD_CPPFLAGS\)\s*$' },
    [pscustomobject]@{ Name = 'Legacy AT build flags consumed'; Pass = $readme -match '(?m)^LEGACY_AT_LIBRARY_CPPFLAGS\s*:=\s*\$\(BTSTACK_AT_BUILD_CPPFLAGS\)\s*$' },
    [pscustomobject]@{ Name = 'port receives target flags'; Pass = $readme -match '(?s)every compilation unit.*selected port' }
)
foreach ($check in $documentationChecks) {
    if (-not $check.Pass) { throw "Documentation missing: $($check.Name)" }
}

$build = Join-Path ([IO.Path]::GetTempPath()) (
    'btstack_selected_build_' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $build | Out-Null
try {
    $source = Join-Path $repoRoot 'btstack\ports\gpcv2178\btstack_rtos_gpcv2178.c'
    $portConfig = Join-Path $repoRoot 'btstack\ports\gpcv2178\btstack_gpcv2178_config.h'
    $includeArgs = @(
        '-I', (Join-Path $repoRoot 'btstack\core\tests\rtos_gpcv2178_stubs'),
        '-I', (Join-Path $repoRoot 'btstack\ports\gpcv2178'),
        '-I', (Join-Path $repoRoot 'btstack\include')
    )
    $objects = @{
        API = Join-Path $build 'rtos_api.o'
        AT = Join-Path $build 'rtos_at.o'
    }
    $variantMatrix = @(
        [pscustomobject]@{ Name = 'neither'; Defines = @(); Pass = $false },
        [pscustomobject]@{ Name = 'both'; Defines = @(
                "-D$apiDefine=1", "-D$atDefine=1"); Pass = $false },
        [pscustomobject]@{ Name = 'API zero'; Defines = @(
                "-D$apiDefine=0"); Pass = $false },
        [pscustomobject]@{ Name = 'AT zero'; Defines = @(
                "-D$atDefine=0"); Pass = $false },
        [pscustomobject]@{ Name = 'API one'; Defines = @(
                "-D$apiDefine=1"); Pass = $true },
        [pscustomobject]@{ Name = 'AT one'; Defines = @(
                "-D$atDefine=1"); Pass = $true }
    )
    foreach ($case in $variantMatrix) {
        $matrixObject = Join-Path $build (
            'matrix_' + $case.Name.Replace(' ', '_') + '.o')
        $matrixError = Join-Path $build (
            'matrix_' + $case.Name.Replace(' ', '_') + '.stderr.txt')
        $savedErrorPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & $gcc -std=c99 -Wall -Wextra -Werror -fdata-sections `
                @($case.Defines) @includeArgs -include $portConfig `
                -c $source -o $matrixObject `
                2>$matrixError
            $compiled = $LASTEXITCODE -eq 0
        } finally {
            $ErrorActionPreference = $savedErrorPreference
        }
        if ($compiled -ne $case.Pass) {
            throw "Build variant matrix '$($case.Name)' compiled=$compiled expected=$($case.Pass)"
        }
        $matrixErrorText = if (Test-Path -LiteralPath $matrixError) {
            Get-Content -Raw -LiteralPath $matrixError
        } else { '' }
        if (-not $case.Pass) {
            $diagnosticCount = [regex]::Matches(
                $matrixErrorText, '(?m):\s+error:').Count
            if (($diagnosticCount -ne 1) -or
                ($matrixErrorText -notmatch
                    'Define exactly one BTstack build variant with value 1')) {
                throw "Build variant matrix '$($case.Name)' emitted secondary diagnostics: $matrixErrorText"
            }
        }
    }
    & $gcc -std=c99 -Wall -Wextra -Werror -fdata-sections `
        "-D$apiDefine=1" @includeArgs -include $portConfig `
        -c $source -o $objects.API
    if ($LASTEXITCODE -ne 0) { throw 'API-selected port compile failed' }
    & $gcc -std=c99 -Wall -Wextra -Werror -fdata-sections `
        "-D$atDefine=1" @includeArgs -include $portConfig `
        -c $source -o $objects.AT
    if ($LASTEXITCODE -ne 0) { throw 'AT-selected port compile failed' }

    $apiSymbols = (& $nmTool -S $objects.API) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'API-selected nm inspection failed' }
    $atSymbols = (& $nmTool -S $objects.AT) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'AT-selected nm inspection failed' }

    if (($apiSymbols -notmatch '(?m)\b_?btstack_api_stack$') -or
        ($apiSymbols -notmatch '(?m)\b_?btstack_event_stack$')) {
        throw 'API-selected port must retain API and EVENT static stacks'
    }
    if ($apiSymbols -match '(?m)\b_?btstack_at_stack$') {
        throw 'API-selected port retains the AT static stack'
    }
    if (($apiSymbols -notmatch '(?m)\b_?btstack_api_control$') -or
        ($apiSymbols -notmatch '(?m)\b_?btstack_event_control$')) {
        throw 'API-selected port must retain API and EVENT task controls'
    }
    if ($apiSymbols -match '(?m)\b_?btstack_at_control$') {
        throw 'API-selected port retains the AT task control'
    }
    if (($atSymbols -match '(?m)\b_?btstack_api_stack$') -or
        ($atSymbols -match '(?m)\b_?btstack_event_stack$')) {
        throw 'AT-selected port retains API or EVENT static stacks'
    }
    if (($atSymbols -match '(?m)\b_?btstack_api_control$') -or
        ($atSymbols -match '(?m)\b_?btstack_event_control$')) {
        throw 'AT-selected port retains API or EVENT task controls'
    }
    if ($atSymbols -notmatch '(?m)\b_?btstack_at_control$') {
        throw 'AT-selected port must retain one AT task control'
    }
    if ($atSymbols -notmatch '(?m)\b_?btstack_at_stack$') {
        throw 'AT-selected port must retain exactly one AT static stack symbol'
    }
    $apiSections = (& $objdumpTool -h $objects.API) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'API-selected section inspection failed' }
    $atSections = (& $objdumpTool -h $objects.AT) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'AT-selected section inspection failed' }
    $atStackMatch = [regex]::Match($atSections,
        '(?m)^\s*\d+\s+\.data\$btstack_at_stack\s+([0-9a-fA-F]+)\s+')
    if (-not $atStackMatch.Success) {
        throw 'AT-selected port must retain one independently sized AT static stack section'
    }
    $atStackBytes = [Convert]::ToUInt32($atStackMatch.Groups[1].Value, 16)
    if ($atStackBytes -ne 8192) {
        throw "AT static stack bytes=$atStackBytes, expected=8192"
    }
    $apiStaticBytes =
        (Get-ObjectSectionSize $apiSections 'workers') +
        (Get-ObjectSectionSize $apiSections 'btstack_api_stack') +
        (Get-ObjectSectionSize $apiSections 'btstack_event_stack') +
        (Get-ObjectSectionSize $apiSections 'btstack_api_control') +
        (Get-ObjectSectionSize $apiSections 'btstack_event_control')
    $atStaticBytes =
        (Get-ObjectSectionSize $atSections 'workers') +
        (Get-ObjectSectionSize $atSections 'btstack_at_stack') +
        (Get-ObjectSectionSize $atSections 'btstack_at_control')
    if ($apiStaticBytes -ne 12392) {
        throw "Host API static worker RAM=$apiStaticBytes, expected=12392"
    }
    if ($atStaticBytes -ne 8292) {
        throw "Host AT static worker RAM=$atStaticBytes, expected=8292"
    }
} finally {
    if (Test-Path -LiteralPath $build) {
        Remove-Item -LiteralPath $build -Recurse -Force
    }
}

Write-Output 'PASS: API and AT selected builds own disjoint frontends and static worker resources'
