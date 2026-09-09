param(
    [string] $Compiler,
    [string] $CppCompiler,
    [string] $Make
)

$ErrorActionPreference = 'Stop'

if ($Compiler) {
    $gcc = $Compiler
}
elseif (Test-Path -LiteralPath 'D:\mingw32\bin\gcc.exe') {
    $gcc = 'D:\mingw32\bin\gcc.exe'
}
else {
    $gcc = (Get-Command gcc.exe -ErrorAction Stop).Source
}

if ($CppCompiler) {
    $gxx = $CppCompiler
}
elseif (Test-Path -LiteralPath 'D:\mingw32\bin\g++.exe') {
    $gxx = 'D:\mingw32\bin\g++.exe'
}
else {
    $gxx = (Get-Command g++.exe -ErrorAction Stop).Source
}

if ($Make) {
    $makeTool = $Make
}
elseif (Test-Path -LiteralPath 'D:\mingw32\bin\mingw32-make.exe') {
    $makeTool = 'D:\mingw32\bin\mingw32-make.exe'
}
else {
    $makeTool = (Get-Command mingw32-make.exe -ErrorAction Stop).Source
}

$testsDir = $PSScriptRoot
$coreDir = Split-Path -Parent $testsDir
$btstackRoot = Split-Path -Parent $coreDir
$buildDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ('btstack_platform_' + [System.Guid]::NewGuid().ToString('N'))
$exe = Join-Path $buildDir 'test_btstack_platform.exe'
$mutantExe = Join-Path $buildDir 'test_btstack_platform_mutant.exe'
$mutantSource = Join-Path $buildDir 'btstack_platform_mutant.c'
$mutantStdout = Join-Path $buildDir 'mutant.stdout.txt'
$mutantStderr = Join-Path $buildDir 'mutant.stderr.txt'
$headerC = Join-Path $buildDir 'test_btstack_platform_header.c'
$headerCpp = Join-Path $buildDir 'test_btstack_platform_header.cpp'
$makeFixture = Join-Path $buildDir 'Makefile'
$platformHeader = Join-Path $btstackRoot 'include\port\btstack_platform.h'

New-Item -ItemType Directory -Path $buildDir | Out-Null
try {
    $platformHeaderText = Get-Content -Raw -LiteralPath $platformHeader
    if (($platformHeaderText -notmatch 'static or application-owned') -or
        ($platformHeaderText -notmatch 'process or firmware') -or
        ($platformHeaderText -notmatch
            '(?s)late_deinit.*must not release.*descriptor.*nested ops.*callback contexts')) {
        throw 'Platform header is missing the process-lifetime descriptor contract'
    }
    Set-Content -LiteralPath $headerC -Encoding Ascii -Value @'
#include "port/btstack_platform.h"
int main(void) { return BTSTACK_PLATFORM_ABI_VERSION == 1u ? 0 : 1; }
'@
    Set-Content -LiteralPath $headerCpp -Encoding Ascii -Value @'
#include "port/btstack_platform.h"
int main() { return BTSTACK_PLATFORM_ABI_VERSION == 1u ? 0 : 1; }
'@

    $makefileInc = (Join-Path $btstackRoot 'Makefile.inc').Replace('\', '/')
    $portMakefileInc = (Join-Path $btstackRoot 'ports\gpcv2178\Makefile.inc').Replace('\', '/')
    $afterMakeInclude = (Join-Path $btstackRoot 'src\Makefile.inc').Replace('\', '/')
    Set-Content -LiteralPath $makeFixture -Encoding Ascii -Value @(
        "include $makefileInc",
        "include $portMakefileInc",
        "include $afterMakeInclude",
        '$(info __ROOT__$(BTSTACK_ROOT))',
        '$(foreach item,$(BTSTACK_INCLUDE_DIRS),$(info __INCLUDE__$(item)))',
        '$(foreach item,$(BTSTACK_COMMON_SOURCES),$(info __COMMON__$(item)))',
        '$(foreach item,$(BTSTACK_SOURCES),$(info __SOURCE__$(item)))',
        '$(foreach item,$(BTSTACK_AT_CORE_SOURCES),$(info __AT_CORE__$(item)))',
        '$(foreach item,$(BTSTACK_AT_FRONTEND_SOURCES),$(info __AT__$(item)))',
        '$(foreach item,$(BTSTACK_API_FRONTEND_SOURCES),$(info __API__$(item)))',
        '$(foreach item,$(BTSTACK_GPCV2178_PORT_SOURCES),$(info __PORT__$(item)))',
        'all:',
        "`t@echo manifest-read")

    $makeOutput = @(& $makeTool --no-print-directory -f $makeFixture 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack Makefile.inc evaluation failed: $($makeOutput -join [Environment]::NewLine)"
    }
    $resolvedRootText = [string](@($makeOutput | Where-Object {
                $_.ToString().StartsWith('__ROOT__')
            })[0]).Substring('__ROOT__'.Length)
    $resolvedRoot = [IO.Path]::GetFullPath($resolvedRootText)
    if ($resolvedRoot -ne [IO.Path]::GetFullPath($btstackRoot)) {
        throw "BTSTACK_ROOT resolved to '$resolvedRoot', expected '$btstackRoot'"
    }

    $callerRoot = (Join-Path $buildDir 'caller-root').Replace('\', '/')
    $overrideOutput = @(& $makeTool --no-print-directory -f $makeFixture `
            "BTSTACK_ROOT=$callerRoot" 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "BTSTACK_ROOT caller override evaluation failed: $($overrideOutput -join [Environment]::NewLine)"
    }
    $overriddenRoot = [string](@($overrideOutput | Where-Object {
                $_.ToString().StartsWith('__ROOT__')
            })[0]).Substring('__ROOT__'.Length)
    if ([IO.Path]::GetFullPath($overriddenRoot) -ne
        [IO.Path]::GetFullPath($callerRoot)) {
        throw "BTSTACK_ROOT caller override was not preserved: $overriddenRoot"
    }

    $includeDirs = @($makeOutput | Where-Object {
            $_.ToString().StartsWith('__INCLUDE__')
        } | ForEach-Object {
            [IO.Path]::GetFullPath($_.ToString().Substring('__INCLUDE__'.Length))
        })
    foreach ($requiredInclude in @(
            (Join-Path $btstackRoot 'src'),
            (Join-Path $btstackRoot 'transport'))) {
        if ([IO.Path]::GetFullPath($requiredInclude) -notin $includeDirs) {
            throw "BTSTACK_INCLUDE_DIRS is missing: $requiredInclude"
        }
    }
    foreach ($includeDir in $includeDirs) {
        if (-not (Test-Path -LiteralPath $includeDir -PathType Container)) {
            throw "BTSTACK_INCLUDE_DIRS contains a missing directory: $includeDir"
        }
    }

    function Get-ExpandedPaths([object[]]$output, [string]$prefix) {
        return @($output | Where-Object {
                $_.ToString().StartsWith($prefix)
            } | ForEach-Object {
                [IO.Path]::GetFullPath($_.ToString().Substring($prefix.Length))
            })
    }
    function Assert-ManifestSetContract(
        [string[]]$common,
        [string[]]$alias,
        [string[]]$atFrontend,
        [string[]]$apiFrontend) {
        if (@($common | Sort-Object -Unique).Count -ne $common.Count) {
            throw 'BTSTACK_COMMON_SOURCES contains duplicates'
        }
        if ((Compare-Object $common $alias).Count -ne 0) {
            throw 'Deprecated BTSTACK_SOURCES alias must equal BTSTACK_COMMON_SOURCES'
        }
        foreach ($frontend in @($atFrontend, $apiFrontend)) {
            if (@($frontend | Sort-Object -Unique).Count -ne $frontend.Count) {
                throw 'Frontend source set contains duplicates'
            }
            if (@($frontend | Where-Object { $_ -in $common }).Count -ne 0) {
                throw 'Common source set must not contain frontend sources'
            }
        }
        if (@($atFrontend | Where-Object { $_ -in $apiFrontend }).Count -ne 0) {
            throw 'AT and API frontend source sets must be mutually exclusive'
        }
        if (@($common | Where-Object {
                    $_.Replace('\', '/') -match '/ports/gpcv2178/'
                }).Count -ne 0) {
            throw 'Common source set must not select a concrete port'
        }
    }

    $commonSources = Get-ExpandedPaths $makeOutput '__COMMON__'
    $sources = Get-ExpandedPaths $makeOutput '__SOURCE__'
    $atCoreSources = Get-ExpandedPaths $makeOutput '__AT_CORE__'
    $atFrontendSources = Get-ExpandedPaths $makeOutput '__AT__'
    $apiFrontendSources = Get-ExpandedPaths $makeOutput '__API__'
    $portSources = Get-ExpandedPaths $makeOutput '__PORT__'
    Assert-ManifestSetContract $commonSources $sources `
        $atFrontendSources $apiFrontendSources
    $mutationRejected = $false
    try {
        Assert-ManifestSetContract @($commonSources + $atFrontendSources[0]) `
            $sources $atFrontendSources $apiFrontendSources
    }
    catch {
        $mutationRejected = $true
    }
    if (-not $mutationRejected) {
        throw 'Frontend-in-common mutation bait was not rejected'
    }
    foreach ($requiredSource in @(
            'core\btstack_platform.c',
            'core\btstack_ring.c',
            'core\btstack_call_audio_tx.c',
            'core\btstack_core_dispatcher.c',
            'core\btstack_event_mapper.c',
            'core\btstack_runtime.c')) {
        $expected = [IO.Path]::GetFullPath((Join-Path $btstackRoot $requiredSource))
        if ($expected -notin $commonSources) {
            throw "BTSTACK_COMMON_SOURCES is missing: $requiredSource"
        }
    }
    $legacyEventSource = [IO.Path]::GetFullPath((Join-Path $btstackRoot `
        'core\btstack_event.c'))
    $eventQueueSource = [IO.Path]::GetFullPath((Join-Path $btstackRoot `
        'core\btstack_event_queue.c'))
    if (Test-Path -LiteralPath $legacyEventSource -PathType Leaf) {
        throw 'btstack_event.c must be merged into btstack_event_queue.c'
    }
    if ($legacyEventSource -in $commonSources) {
        throw 'BTSTACK_COMMON_SOURCES still contains btstack_event.c'
    }
    $eventQueueText = Get-Content -Raw -LiteralPath $eventQueueSource
    if ([regex]::Matches($eventQueueText,
            '\bint\s+btstack_event_init\s*\(\s*void\s*\)').Count -ne 1) {
        throw 'btstack_event_queue.c must own btstack_event_init'
    }
    foreach ($source in @($commonSources + $atCoreSources +
            $atFrontendSources + $apiFrontendSources)) {
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Expanded BTstack manifest contains a missing source: $source"
        }
    }

    $repositoryRoot = Split-Path -Parent $btstackRoot
    $expectedAtCoreSources = @(
        'btstack_at.c',
        'btstack_at_command_table.c',
        'btstack_at_dispatch_table.c',
        'btstack_at_parser.c',
        'btstack_at_protocol.c',
        'btstack_at_core_adapter.c'
    ) | ForEach-Object {
        [IO.Path]::GetFullPath((Join-Path $btstackRoot "at\$_"))
    }
    $expectedAtFrontendSources = @($expectedAtCoreSources +
        [IO.Path]::GetFullPath((Join-Path $btstackRoot `
            'at\bt_xstack_at_frontend.c')))
    $expectedApiFrontendSources = @(
        [IO.Path]::GetFullPath((Join-Path $btstackRoot `
            'api\bt_xstack_api_core.c')))
    $projectContracts = @(
            [PSCustomObject]@{
                Name = 'API';
                Path = 'Project\WirelessLibraries\BTstack_API_Library\BTstack_API_Library.gpcmprj';
                Frontend = $expectedApiFrontendSources;
                ForbiddenFrontend = $expectedAtFrontendSources
            },
            [PSCustomObject]@{
                Name = 'AT';
                Path = 'Project\WirelessLibraries\BTstack_AT_Library\BTstack_AT_Library.gpcmprj';
                Frontend = $expectedAtFrontendSources;
                ForbiddenFrontend = $expectedApiFrontendSources
            },
            [PSCustomObject]@{
                Name = 'Legacy AT';
                Path = 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj';
                Frontend = $expectedAtCoreSources;
                ForbiddenFrontend = @($expectedApiFrontendSources +
                    ($expectedAtFrontendSources | Where-Object {
                        $_ -notin $expectedAtCoreSources
                    }))
            })
    foreach ($projectContract in $projectContracts) {
        $projectPath = Join-Path $repositoryRoot $projectContract.Path
        [xml]$projectXml = Get-Content -Raw -LiteralPath $projectPath
        $projectDirectory = Split-Path -Parent $projectPath
        $projectSources = @($projectXml.SelectNodes('//Unit') | ForEach-Object {
                [IO.Path]::GetFullPath((Join-Path $projectDirectory ([string]$_.filename)))
            })
        if (@($projectSources | Sort-Object -Unique).Count -ne $projectSources.Count) {
            throw "$($projectContract.Name) project contains duplicate units"
        }
        $declaredSources = @($commonSources + $projectContract.Frontend + $portSources)
        $invalidDeclaredSources = @($declaredSources | Where-Object {
                $source = $_
                @($projectSources | Where-Object { $_ -eq $source }).Count -ne 1
            })
        if ($invalidDeclaredSources.Count -ne 0) {
            throw "$($projectContract.Name) project must consume declared sources exactly once: $($invalidDeclaredSources -join ', ')"
        }
        foreach ($source in $projectContract.ForbiddenFrontend) {
            if ($projectSources -contains $source) {
                throw "$($projectContract.Name) project consumes the wrong frontend: $source"
            }
        }
        $expectedCommon = @($projectSources | Where-Object {
                ($_ -notin $projectContract.Frontend) -and
                ($_ -notin $portSources)
            })
        $projectContract | Add-Member -NotePropertyName ExpectedCommon `
            -NotePropertyValue $expectedCommon
    }

    $expectedCommon = @($projectContracts[0].ExpectedCommon)
    foreach ($projectContract in $projectContracts | Select-Object -Skip 1) {
        $difference = @(Compare-Object $expectedCommon `
            @($projectContract.ExpectedCommon))
        if ($difference.Count -ne 0) {
            throw "$($projectContract.Name) project boundary does not yield the same common production set: $($difference.InputObject -join ', ')"
        }
    }
    $missingCommon = @($expectedCommon | Where-Object { $_ -notin $commonSources })
    $extraCommon = @($commonSources | Where-Object { $_ -notin $expectedCommon })
    if (($missingCommon.Count -ne 0) -or ($extraCommon.Count -ne 0)) {
        throw "BTSTACK_COMMON_SOURCES must equal the project-derived common production set (missing=$($missingCommon.Count): $($missingCommon -join ', '); extra=$($extraCommon.Count): $($extraCommon -join ', '))"
    }
    if ((Compare-Object $expectedAtCoreSources $atCoreSources).Count -ne 0) {
        throw 'BTSTACK_AT_CORE_SOURCES must equal the six internal AT sources'
    }
    if ((Compare-Object $expectedAtFrontendSources $atFrontendSources).Count -ne 0) {
        throw 'BTSTACK_AT_FRONTEND_SOURCES must equal AT core plus the public AT frontend'
    }
    if ((Compare-Object $expectedApiFrontendSources $apiFrontendSources).Count -ne 0) {
        throw 'BTSTACK_API_FRONTEND_SOURCES does not match the API project boundary'
    }

    $readmeText = Get-Content -Raw -LiteralPath (Join-Path $btstackRoot 'README.md')
    $documentedMakeBlocks = @([regex]::Matches(
            $readmeText, '(?s)```make\s*(.*?)\s*```'))
    if ($documentedMakeBlocks.Count -ne 1) {
        throw 'BTstack README must contain one selected-frontend Make example'
    }
    $documentedMakeFixture = Join-Path $buildDir 'README.Makefile'
    $documentedMakeText = @(
        "BTSTACK_ROOT := $($btstackRoot.Replace('\', '/'))",
        $documentedMakeBlocks[0].Groups[1].Value,
        '$(foreach item,$(AT_LIBRARY_SOURCES),$(info __DOC_AT__$(item)))',
        '$(foreach item,$(LEGACY_AT_LIBRARY_SOURCES),$(info __DOC_LEGACY__$(item)))',
        '$(foreach item,$(API_LIBRARY_SOURCES),$(info __DOC_API__$(item)))',
        'all:',
        "`t@echo documented-manifest-read"
    )
    Set-Content -LiteralPath $documentedMakeFixture -Encoding Ascii `
        -Value $documentedMakeText
    $documentedMakeOutput = @(& $makeTool --no-print-directory `
            -f $documentedMakeFixture 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "README Make example does not parse: $($documentedMakeOutput -join [Environment]::NewLine)"
    }
    $documentedAt = Get-ExpandedPaths $documentedMakeOutput '__DOC_AT__'
    $documentedLegacy = Get-ExpandedPaths $documentedMakeOutput '__DOC_LEGACY__'
    $documentedApi = Get-ExpandedPaths $documentedMakeOutput '__DOC_API__'
    if ((Compare-Object @($commonSources + $atFrontendSources + $portSources) `
            $documentedAt).Count -ne 0) {
        throw 'README AT Make example does not select COMMON + AT frontend + port'
    }
    if ((Compare-Object @($commonSources + $apiFrontendSources + $portSources) `
            $documentedApi).Count -ne 0) {
        throw 'README API Make example does not select COMMON + API frontend + port'
    }
    if ((Compare-Object @($commonSources + $atCoreSources + $portSources) `
            $documentedLegacy).Count -ne 0) {
        throw 'README Legacy Make example does not select COMMON + AT core + port'
    }
    $portsReadme = Get-Content -Raw -LiteralPath `
        (Join-Path $btstackRoot 'ports\README.md')
    foreach ($token in @('BTSTACK_COMMON_SOURCES', 'BTSTACK_AT_CORE_SOURCES',
            'BTSTACK_AT_FRONTEND_SOURCES', 'BTSTACK_API_FRONTEND_SOURCES')) {
        if (-not $portsReadme.Contains($token)) {
            throw "Ports README is missing manifest composition token: $token"
        }
    }
    foreach ($source in $sources) {
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "BTSTACK_SOURCES contains a missing source: $source"
        }
    }

    $includeArgs = @()
    foreach ($includeDir in $includeDirs) {
        $includeArgs += @('-I', $includeDir)
    }
    $platformSource = [IO.Path]::GetFullPath(
        (Join-Path $btstackRoot 'core\btstack_platform.c'))
    $callAudioSource = [IO.Path]::GetFullPath(
        (Join-Path $btstackRoot 'core\btstack_call_audio_tx.c'))
    $commonCriticalSources = @(
        (Join-Path $coreDir 'btstack_profile_manager.c'),
        (Join-Path $coreDir 'btstack_event_mapper.c'),
        (Join-Path $coreDir 'btstack_property.c'),
        (Join-Path $coreDir 'btstack_event_queue.c'))
    if ($platformSource -notin $sources) {
        throw 'BTSTACK_SOURCES does not contain core/btstack_platform.c'
    }
    if ($callAudioSource -notin $sources) {
        throw 'BTSTACK_SOURCES does not contain core/btstack_call_audio_tx.c'
    }

    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -DBTSTACK_CLASSIC=0 -DBTSTACK_LE=0 `
        -DBTSTACK_GATT_CLIENT=0 -DBTSTACK_GATT_SERVER=0 `
        -DBTSTACK_HOGP_DEVICE=0 `
        -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -ffunction-sections -fdata-sections `
        @includeArgs `
        $platformSource `
        $callAudioSource `
        (Join-Path $coreDir 'btstack_port_ops.c') `
        @commonCriticalSources `
        (Join-Path $testsDir 'test_btstack_platform_link_stubs.c') `
        (Join-Path $testsDir 'test_btstack_platform.c') `
        '-Wl,--gc-sections' `
        -o $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack platform test compilation failed: $LASTEXITCODE"
    }

    $platformSourceText = Get-Content -Raw -LiteralPath $platformSource
    $mutationPattern = '(?s)if \(status != BTSTACK_PLATFORM_OK\) \{\s*return btstack_platform_record_status\(status\);\s*\}'
    $mutationReplacement = @'
if (status != BTSTACK_PLATFORM_OK) {
        (void) btstack_port_ops_set_ops(NULL);
        return btstack_platform_record_status(status);
    }
'@
    $mutantSourceText = [regex]::Replace(
        $platformSourceText,
        $mutationPattern,
        $mutationReplacement,
        1)
    if ($mutantSourceText -eq $platformSourceText) {
        throw 'BTstack platform mutation insertion point was not found'
    }
    Set-Content -LiteralPath $mutantSource -Encoding Ascii -Value $mutantSourceText
    & $gcc -std=c99 -O0 -Wall -Wextra -Werror `
        -DBTSTACK_CLASSIC=0 -DBTSTACK_LE=0 `
        -DBTSTACK_GATT_CLIENT=0 -DBTSTACK_GATT_SERVER=0 `
        -DBTSTACK_HOGP_DEVICE=0 `
        -DHCI_ACL_PAYLOAD_SIZE=1021 `
        -ffunction-sections -fdata-sections `
        @includeArgs `
        $mutantSource `
        $callAudioSource `
        (Join-Path $coreDir 'btstack_port_ops.c') `
        @commonCriticalSources `
        (Join-Path $testsDir 'test_btstack_platform_link_stubs.c') `
        (Join-Path $testsDir 'test_btstack_platform.c') `
        '-Wl,--gc-sections' `
        -o $mutantExe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack platform mutant compilation failed: $LASTEXITCODE"
    }
    $mutantProcess = Start-Process -FilePath $mutantExe `
        -ArgumentList 'null-platform' `
        -Wait -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $mutantStdout `
        -RedirectStandardError $mutantStderr
    if ($mutantProcess.ExitCode -eq 0) {
        throw 'Invalid-path legacy mutation survived the sentinel assertion'
    }

    $invalidActivationCases = @(
        'null-platform',
        'bad-version',
        'bad-size',
        'bad-name',
        'missing-enter-critical',
        'missing-exit-critical',
        'missing-time',
        'missing-worker-validate',
        'missing-worker-start',
        'missing-worker-notify',
        'missing-worker-wait',
        'missing-worker-stop',
        'missing-control',
        'null-control-init',
        'missing-early-init',
        'missing-late-deinit',
        'missing-run-loop-getter',
        'null-run-loop-result',
        'missing-transport-getter',
        'null-transport-result',
        'missing-transport-config-getter',
        'null-transport-config-result',
        'missing-stack-post')
    $failedCases = @()
    foreach ($caseName in $invalidActivationCases) {
        & $exe $caseName
        if ($LASTEXITCODE -ne 0) {
            $failedCases += $caseName
        }
    }
    if ($failedCases.Count -ne 0) {
        throw "BTstack platform invalid activation failed: $($failedCases -join ', ')"
    }

    & $exe 'legacy-critical'
    if ($LASTEXITCODE -ne 0) {
        throw 'BTstack legacy critical fallback behavior failed'
    }

    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "BTstack platform behavior test failed: $LASTEXITCODE"
    }

    & $gcc -std=c99 -Wall -Wextra -Werror -fsyntax-only `
        @includeArgs $headerC
    if ($LASTEXITCODE -ne 0) {
        throw 'BTstack platform C header test failed'
    }

    & $gxx -std=c++11 -Wall -Wextra -Werror -fsyntax-only `
        @includeArgs $headerCpp
    if ($LASTEXITCODE -ne 0) {
        throw 'BTstack platform C++ header test failed'
    }
}
finally {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}
