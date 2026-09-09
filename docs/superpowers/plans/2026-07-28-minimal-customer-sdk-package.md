# Minimal Customer SDK Package Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the broad customer SDK copy with a package containing only the files required to compile and link the current main-project Debug configuration.

**Architecture:** Parse the active Debug project into compilation roots and explicit linker inputs, derive the recursive project-local header closure with Generalplus GCC dependency generation, and stage only that deterministic file set. Generate into a candidate sibling, validate compilation and linking entirely from the candidate, then atomically replace the official customer directory while retaining the previous package as a timestamped backup.

**Tech Stack:** PowerShell 5+, Generalplus G+ project XML, Generalplus ARM Embedded GCC 4.9.3, `arm-none-eabi-gcc/g++/strip/ranlib/ar/nm/objdump`, fixture-based PowerShell contract tests.

---

## File Structure

- Modify `tools/wireless_lib/WirelessLibTools.psm1`
  - Parse active compilation roots and explicit archives.
  - Generate GCC dependency paths.
  - calculate the minimal deterministic file set.
  - Stage and checksum the minimal customer package.
- Modify `tools/wireless_lib/package_wireless_release.ps1`
  - Make minimal staging the default release workflow.
- Modify `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
  - Cover project parsing, dependency filtering, minimal retention, wireless source rejection, stripping, and checksum generation.
- Create `tools/wireless_lib/verify_minimal_customer_build.ps1`
  - Compile and link the candidate using only candidate paths.
- Modify `docs/wireless_static_library_integration.md`
  - Document minimal-package generation and its current-Debug-only support boundary.
- Regenerate `output/wireless_libs/verification-report.txt`
  - Record final counts, hashes, link result, and package audit.

### Task 1: Parse the Current Debug Build Inputs

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] **Step 1: Write failing fixture assertions**

Add a fixture project containing active C, C++, assembly, header units, a
commented wireless C unit, a linker script, and two archives. Assert:

```powershell
$inputs = Get-DebugProjectInputs `
    -ProjectFile $fixtureProject `
    -WorkspaceRoot $fixtureRoot

Assert-Equal 4 $inputs.CompileUnits.Count 'active compile roots'
Assert-Equal 2 $inputs.Archives.Count 'explicit linked archives'
Assert-Equal 1 $inputs.LinkerScripts.Count 'linker script'
Assert-Equal 0 @($inputs.CompileUnits |
    Where-Object { $_ -match '(atbm6165|btstack).+\.c$' }).Count `
    'commented wireless units are not active roots'
```

- [ ] **Step 2: Run the contract test and verify RED**

Run:

```powershell
& tools\wireless_lib\tests\test_wireless_lib_tools.ps1
```

Expected: failure because `Get-DebugProjectInputs` is unavailable.

- [ ] **Step 3: Implement project-input parsing**

Add:

```powershell
function Get-DebugProjectInputs {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][string]$WorkspaceRoot
    )

    $workspace = Get-NormalizedFullPath $WorkspaceRoot (Get-Location).Path
    $project = Get-NormalizedFullPath $ProjectFile (Get-Location).Path
    $projectDirectory = Split-Path -Parent $project
    [xml]$xml = Get-Content -LiteralPath $project -Raw
    $target = @($xml.emIDE_project_file.Project.Build.Target |
        Where-Object { [string]$_.title -eq 'Debug' })[0]

    $compileExtensions = @('.c', '.cpp', '.s')
    $compileUnits = @($xml.emIDE_project_file.Project.Unit |
        Where-Object { $_.HasAttribute('filename') } |
        ForEach-Object {
            $path = Get-NormalizedFullPath ([string]$_.filename) $projectDirectory
            if ($compileExtensions -contains [IO.Path]::GetExtension($path).ToLowerInvariant() -and
                $path.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase)) {
                $path
            }
        } | Sort-Object -Unique)

    $archives = @($target.Linker.Add |
        Where-Object { $_.HasAttribute('library') } |
        ForEach-Object {
            Get-NormalizedFullPath ([string]$_.library) $projectDirectory
        })

    $linkerScripts = @($target.Linker.Add |
        Where-Object { $_.HasAttribute('option') -and [string]$_.option -match '^-T(.+)$' } |
        ForEach-Object { Get-NormalizedFullPath $Matches[1] $projectDirectory })

    [PSCustomObject]@{
        ProjectFile = $project
        ProjectDirectory = $projectDirectory
        CompileUnits = $compileUnits
        Archives = $archives
        LinkerScripts = $linkerScripts
    }
}
```

Preserve `.S` by comparing extensions case-sensitively before lowercasing and
classify it as preprocessed assembly.

- [ ] **Step 4: Run tests and verify GREEN**

Run the same contract test. Expected: all parsing assertions pass.

### Task 2: Derive the Recursive Header Closure

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] **Step 1: Write failing dependency-filter assertions**

Use a fixture dependency output containing a source, two workspace headers,
one toolchain header, and one path outside the workspace:

```powershell
$paths = ConvertFrom-GccDependencyOutput `
    -Output $dependencyOutput `
    -WorkspaceRoot $fixtureRoot

Assert-Equal 3 $paths.Count 'source plus two project-local headers'
Assert-Equal 0 @($paths | Where-Object {
    -not $_.StartsWith($fixtureRoot, [StringComparison]::OrdinalIgnoreCase)
}).Count 'external and toolchain headers excluded'
```

- [ ] **Step 2: Run tests and verify RED**

Expected: `ConvertFrom-GccDependencyOutput` is unavailable.

- [ ] **Step 3: Implement deterministic dependency generation**

Add:

```powershell
function ConvertFrom-GccDependencyOutput {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string[]]$Output,
        [Parameter(Mandatory = $true)][string]$WorkspaceRoot
    )

    $workspace = Get-NormalizedFullPath $WorkspaceRoot (Get-Location).Path
    $joined = ($Output -join ' ') -replace '\\\s+', ' '
    $payload = $joined.Substring($joined.IndexOf(':') + 1)
    @($payload -split '\s+' |
        Where-Object { $_ } |
        ForEach-Object {
            $candidate = [IO.Path]::GetFullPath($_.Replace('/', '\'))
            if ($candidate.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase) -and
                (Test-Path -LiteralPath $candidate -PathType Leaf)) {
                $candidate
            }
        } | Sort-Object -Unique)
}

function Get-GccProjectDependencies {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Inputs,
        [Parameter(Mandatory = $true)]$Configuration,
        [Parameter(Mandatory = $true)][string]$ToolchainDirectory,
        [Parameter(Mandatory = $true)][string]$WorkspaceRoot
    )

    $gcc = Join-Path $ToolchainDirectory 'arm-none-eabi-gcc.exe'
    $gxx = Join-Path $ToolchainDirectory 'arm-none-eabi-g++.exe'
    $includeArguments = @($Configuration.IncludeDirectories |
        ForEach-Object { "-I$_" })
    $results = foreach ($source in $Inputs.CompileUnits) {
        $extension = [IO.Path]::GetExtension($source)
        $compiler = if ($extension -ieq '.cpp') { $gxx } else { $gcc }
        $language = if ($extension -ceq '.S') {
            @('-x', 'assembler-with-cpp')
        } elseif ($extension -ieq '.s') {
            @()
        } else {
            @()
        }
        if ($extension -ieq '.s' -and $extension -cne '.S') {
            @($source)
            continue
        }
        $output = Invoke-CheckedProcess -FilePath $compiler -Arguments @(
            $Configuration.Options + $includeArguments + $language +
            @('-MM', '-MT', 'dependency-target', $source)
        ) -Description "Dependencies for $source" -Quiet
        ConvertFrom-GccDependencyOutput $output $WorkspaceRoot
    }
    @($results | Sort-Object -Unique)
}
```

Handle spaces and GCC continuation lines with a parser test before accepting
the implementation.

- [ ] **Step 4: Run tests and verify GREEN**

Expected: dependency parsing and workspace filtering pass.

### Task 3: Calculate and Stage the Minimal File Set

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] **Step 1: Write failing minimal-stage test**

Create fixture paths for one active source, one used header, one unused
header, one linked archive, one unlinked archive, the main project, and the
linker script. Stage with an injected dependency list and assert:

```powershell
$result = New-MinimalWirelessCustomerStage `
    -WorkspaceRoot $fixtureRoot `
    -StageRoot $stageRoot `
    -ProjectFile $fixtureProject `
    -DependencyPaths @($usedHeader) `
    -CustomerStripTool $fakeStrip `
    -CustomerRanlibTool $fakeRanlib

Assert-True (Test-Path (Join-Path $stageRoot 'src\active.c')) 'active source'
Assert-True (Test-Path (Join-Path $stageRoot 'inc\used.h')) 'used header'
Assert-True (-not (Test-Path (Join-Path $stageRoot 'inc\unused.h'))) 'unused header removed'
Assert-True (Test-Path (Join-Path $stageRoot 'lib\linked.a')) 'linked archive'
Assert-True (-not (Test-Path (Join-Path $stageRoot 'lib\unused.a'))) 'unlinked archive removed'
```

- [ ] **Step 2: Run tests and verify RED**

Expected: `New-MinimalWirelessCustomerStage` is unavailable.

- [ ] **Step 3: Implement minimal staging**

Add `Get-WorkspaceRelativePath`, `Copy-MinimalFileSet`, and
`New-MinimalWirelessCustomerStage`. Its keep set is:

```powershell
$keepFiles = @(
    $inputs.ProjectFile
    $inputs.CompileUnits
    $inputs.Archives
    $inputs.LinkerScripts
    $DependencyPaths
) | Sort-Object -Unique
```

Reject any keep path outside the workspace, any missing path, and any path
matching:

```powershell
'[\\/](btstack|atbm6165)[\\/].+\.c$'
```

Copy each file to the same workspace-relative path under the stage. Strip and
reindex only `libatbm_wifi.a` and `libbtstack.a`. Remove empty directories by
never creating them. Generate and verify `SHA256SUMS.txt` after stripping.

- [ ] **Step 4: Run tests and verify GREEN**

Expected: the unused fixture header and archive are absent, all checksums
verify, and both wireless archives invoke strip and ranlib.

### Task 4: Make Minimal Packaging the Release Default

**Files:**
- Modify: `tools/wireless_lib/package_wireless_release.ps1`
- Modify: `docs/wireless_static_library_integration.md`
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`

- [ ] **Step 1: Write failing script-contract assertions**

Assert the release script calls `Get-DebugProjectInputs`,
`Get-GccProjectDependencies`, and `New-MinimalWirelessCustomerStage`, and no
longer calls `New-WirelessCustomerStage`.

- [ ] **Step 2: Run tests and verify RED**

Expected: release-script contract fails.

- [ ] **Step 3: Update release orchestration**

The release script must:

```powershell
$projectFile = Join-Path $WorkspaceRoot `
    'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$inputs = Get-DebugProjectInputs $projectFile $WorkspaceRoot
$configuration = Get-DebugCompilerConfiguration $projectFile
$dependencies = Get-GccProjectDependencies `
    $inputs $configuration $ToolchainDirectory $WorkspaceRoot
$result = New-MinimalWirelessCustomerStage `
    -WorkspaceRoot $WorkspaceRoot `
    -StageRoot $StageRoot `
    -ProjectFile $projectFile `
    -DependencyPaths $dependencies `
    -CustomerStripTool $stripTool `
    -CustomerRanlibTool $ranlibTool
```

Document that the output supports the current Debug configuration only and
that unused headers/modules are intentionally absent.

- [ ] **Step 4: Run both contract suites**

Run:

```powershell
& tools\wireless_lib\tests\test_wireless_lib_tools.ps1
& tools\wireless_lib\tests\test_wireless_library_projects.ps1
```

Expected: both report `PASS`.

### Task 5: Add Candidate-Only Compile and Link Verification

**Files:**
- Create: `tools/wireless_lib/verify_minimal_customer_build.ps1`
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`

- [ ] **Step 1: Write failing verifier-contract assertions**

Assert the verifier rejects a candidate whose compile unit, dependency
header, linked archive, or linker script resolves outside the candidate root.

- [ ] **Step 2: Run tests and verify RED**

Expected: verifier script or path-containment helper is unavailable.

- [ ] **Step 3: Implement the verifier**

The verifier reparses the candidate project, remaps all project-local include
directories into the candidate, compiles all active roots to
`output/wireless_libs/minimal-customer-verify/obj`, and links them with:

```powershell
@(
    '-Wl,-Map', "-Wl,$mapPath", '-Wl,--gc-sections', '-n', '-Wl,-cref',
    '-mcpu=cortex-a7', "-T$linkerScript", '-specs=nosys.specs',
    '-mfpu=neon-vfpv4', '-mfloat-abi=softfp'
) + $objectPaths + $archivePaths + @('-lm', '-o', $elfPath)
```

Use `arm-none-eabi-g++` for `.cpp`, GCC with
`-x assembler-with-cpp` for `.S`, and GCC for C/assembly. Fail if any
candidate input escapes the candidate root.

- [ ] **Step 4: Run verifier on the candidate**

Run:

```powershell
& tools\wireless_lib\verify_minimal_customer_build.ps1 `
    -CustomerRoot D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer_candidate
```

Expected: 309 compile units, 20 archives, successful ELF, and definitions for
`atbm_start_ap`, `btstack_gpcv2178_start`, and
`btstack_run_loop_gpcv2178_get_instance`.

### Task 6: Generate, Audit, and Replace the Official Package

**Files:**
- Regenerate: sibling `_customer_candidate`
- Regenerate: sibling `_customer`
- Modify: `output/wireless_libs/verification-report.txt`

- [ ] **Step 1: Generate a clean candidate**

Run:

```powershell
& tools\wireless_lib\package_wireless_release.ps1 `
    -StageRoot D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer_candidate
```

Expected: candidate generated with fewer files and headers than the current
1911-file/1060-header package.

- [ ] **Step 2: Run full candidate verification**

Run the verifier, archive member/debug/index audit, wireless-source scan,
exact archive count check, and checksum verification. Expected: zero
failures.

- [ ] **Step 3: Safely replace the official directory**

Resolve and verify both paths are direct children of
`D:\Project\Platform\Generalplus`, then:

```powershell
Move-Item -LiteralPath $official -Destination $timestampedBackup
Move-Item -LiteralPath $candidate -Destination $official
```

Do not delete the backup.

- [ ] **Step 4: Re-run verification from the official path**

Run both contract suites and the candidate-only compile/link verifier against
the official `_customer` path. Expected: all pass.

- [ ] **Step 5: Update the verification report**

Record final file/header counts, bytes, archive member counts and hashes,
wireless source count, compile/link counts, required symbols, checksum count,
and backup path.

- [ ] **Step 6: Record the no-VCS checkpoint**

`git rev-parse --git-dir` must report that the workspace is not a Git
repository. Do not attempt a commit; list the existing files modified and
the new files created in the final handoff.
