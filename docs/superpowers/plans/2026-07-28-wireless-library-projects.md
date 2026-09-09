# Dedicated Wireless Library Projects Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create two independent G+ IDE Debug static-library projects that produce `libatbm_wifi.a` and `libbtstack.a` from the approved current wireless sources.

**Architecture:** A deterministic PowerShell generator reads the approved wireless manifest and the main Debug target, then writes two native `type="0"` G+ project files with relative paths. Contract tests compare every project unit, option, include directory, output, and target against the source-of-truth before native IDE builds and the main integration link are verified.

**Tech Stack:** PowerShell 5, XML, G+ IDE `.gpcmprj`, Generalplus ARM Embedded GCC 4.9.3, GNU `ar`/`nm`, existing wireless build validation.

---

## File Map

- Create `tools/wireless_lib/generate_wireless_projects.ps1`: generate the two native G+ projects from the approved manifest and main Debug configuration.
- Create `tools/wireless_lib/tests/test_wireless_library_projects.ps1`: validate project type, targets, outputs, sources, options, includes, and BTstack target preinclude.
- Create `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`: native WiFi static-library project.
- Create `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`: native BTstack static-library project.
- Modify `tools/wireless_lib/WirelessLibTools.psm1`: exclude internal library projects from customer staging.
- Modify `docs/wireless_static_library_integration.md`: describe G+ projects as the primary internal build path.
- Regenerate `output/customer_wireless_sdk`: customer SDK without internal projects or wireless C sources.
- Update `output/wireless_libs/verification-report.txt`: record dedicated-project validation and builds.

### Task 1: Project contract tests

**Files:**
- Create: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`

- [ ] **Step 1: Write failing project-presence and target tests**

The test imports the existing module, loads the manifest, and checks:

```powershell
$wifiProjectPath = Join-Path $workspace 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
$btProjectPath = Join-Path $workspace 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj'
Assert-True (Test-Path -LiteralPath $wifiProjectPath) 'WiFi G+ project exists'
Assert-True (Test-Path -LiteralPath $btProjectPath) 'BTstack G+ project exists'
```

For each parsed project:

```powershell
Assert-Equal 1 @($project.emIDE_project_file.Project.Build.Target).Count 'Debug target only'
Assert-Equal 'Debug' ([string]$target.title) 'target title'
Assert-Equal '0' ([string]$target.Option | Where-Object { $_.HasAttribute('type') }).type 'static library type'
Assert-Equal 'armelfgcc' ([string]$target.Option | Where-Object { $_.HasAttribute('compiler') }).compiler 'toolchain'
```

- [ ] **Step 2: Add source and configuration equivalence tests**

Resolve each project unit against its project directory and compare sorted
absolute paths with the manifest:

```powershell
Assert-SequenceEqual $manifest.Wifi $wifiUnits 'WiFi project sources'
Assert-SequenceEqual $manifest.Bluetooth $btUnits 'BTstack project sources'
Assert-Equal 92 $wifiUnits.Count 'WiFi project source count'
Assert-Equal 57 $btUnits.Count 'BTstack project source count'
```

Compare all main Debug options and resolved include directories with both
projects. Require BTstack's additional option:

```text
-include ../../../btstack/ports/gpcv2178/btstack_config.h
```

Require output basenames `../../../lib/atbm_wifi` and
`../../../lib/btstack`, with automatic prefix and extension enabled.

- [ ] **Step 3: Run the test and confirm RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Expected: failure because both dedicated project files are missing.

### Task 2: Deterministic native project generation

**Files:**
- Create: `tools/wireless_lib/generate_wireless_projects.ps1`
- Create: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Create: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`

- [ ] **Step 1: Implement XML project generation**

The generator imports `WirelessLibTools.psm1`, calls
`Read-WirelessSourceManifest` and `Get-DebugCompilerConfiguration`, and writes
this structure for each project:

```xml
<emIDE_project_file>
  <FileVersion major="1" minor="7" />
  <Project>
    <Option title="ATBM_WiFi_Library" />
    <Option execution_dir="." />
    <Option pch_mode="2" />
    <Option compiler="armelfgcc" />
    <Option DownloadLog="-1" />
    <Build>
      <Target title="Debug">
        <Option output="../../../lib/atbm_wifi" prefix_auto="1" extension_auto="1" />
        <Option working_dir="" />
        <Option object_output="obj/Debug/" />
        <Option type="0" />
        <Option compiler="armelfgcc" />
        <Option createDefFile="1" />
        <Compiler />
        <Linker />
        <PostOutput>
          <ConvertedOutput PostOutputCreate="0" />
          <ConvertedOutput PostOutputType="HEX" />
          <ConvertedOutput PostOutputFile="${TARGET_OUTPUT_DIR}${TARGET_OUTPUT_BASENAME}" />
        </PostOutput>
      </Target>
    </Build>
  </Project>
</emIDE_project_file>
```

Populate `<Compiler>` with every main Debug `<Add option>` and each include
directory re-relativized to the dedicated project directory. Add the BTstack
preinclude after the inherited options.

- [ ] **Step 2: Populate source units**

For every manifest source, append a project-relative unit:

```xml
<Unit filename="../../../gplib/liba_wifi_src/atbm/atbm6165/api/atbm_api.c">
  <Option compilerVar="CC" />
</Unit>
```

Use `../../../btstack/...` paths for the Bluetooth project. Reject output paths
outside `Project/WirelessLibraries`, missing sources, duplicates, or counts
other than 92 and 57.

- [ ] **Step 3: Generate both project files**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/generate_wireless_projects.ps1
```

Expected:

```text
Generated ATBM_WiFi_Library.gpcmprj with 92 source units
Generated BTstack_Library.gpcmprj with 57 source units
```

- [ ] **Step 4: Run project contract tests and confirm GREEN**

Run Task 1's test command. Expected: `PASS: dedicated wireless G+ project contract tests`.

### Task 3: Build and application integration verification

**Files:**
- Generated: `Project/WirelessLibraries/ATBM_WiFi_Library/obj/Debug/**`
- Generated: `Project/WirelessLibraries/BTstack_Library/obj/Debug/**`
- Generated: `lib/libatbm_wifi.a`
- Generated: `lib/libbtstack.a`
- Update: `output/wireless_libs/verification-report.txt`

- [ ] **Step 1: Build both Debug targets in G+ IDE**

Open each `.gpcmprj`, select `Debug`, and invoke Build. The native project type
must call `arm-none-eabi-gcc.exe` for compilation and
`arm-none-eabi-ar.exe` for static archive output.

If GUI automation is unavailable, verify project-driven compilation by parsing
the generated `.gpcmprj` and invoking the same configured GCC 4.9.3 tools from
its units/options/includes; record that limitation explicitly rather than
claiming an IDE GUI build.

- [ ] **Step 2: Audit archives**

Run:

```powershell
arm-none-eabi-ar.exe t lib/libatbm_wifi.a
arm-none-eabi-ar.exe t lib/libbtstack.a
arm-none-eabi-nm.exe -s lib/libatbm_wifi.a
arm-none-eabi-nm.exe -s lib/libbtstack.a
```

Expected: 92 and 57 members, readable archive indexes, and exported ATBM and
BTstack entry points.

- [ ] **Step 3: Link the main Debug application**

Compile the 309 active non-wireless main-project units and link them with all
20 project libraries, including the two newly generated archives. Require exit
code 0 and final ELF definitions for:

```text
atbm_start_ap
btstack_gpcv2178_start
btstack_run_loop_gpcv2178_get_instance
```

### Task 4: Customer staging and documentation

**Files:**
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `docs/wireless_static_library_integration.md`
- Regenerate: `output/customer_wireless_sdk`
- Update: `output/wireless_libs/verification-report.txt`

- [ ] **Step 1: Add a failing stage-exclusion assertion**

Extend the customer-stage fixture and assert:

```powershell
Assert-True (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'Project\WirelessLibraries'))) 'internal wireless library projects excluded'
```

Expected before implementation: failure because the generic SDK copy includes
that directory.

- [ ] **Step 2: Exclude internal projects from customer staging**

Add this exact directory to `New-WirelessCustomerStage` exclusions:

```powershell
(Join-Path $workspace 'Project\WirelessLibraries')
```

Run the staging contract test and require PASS.

- [ ] **Step 3: Update integration documentation**

Document opening and building the two `.gpcmprj` files as the primary internal
workflow. Retain the PowerShell build command only as a reproducibility
cross-check. State that the customer receives neither project nor wireless C
source.

- [ ] **Step 4: Regenerate and verify the customer package**

Create a fresh customer stage path, verify both `.a` files and required headers,
verify zero staged ATBM/BTstack `.c` files, verify
`Project/WirelessLibraries` is absent, and verify every checksum entry.

### Task 5: Final checks and no-VCS handoff

**Files:**
- Update: `output/wireless_libs/verification-report.txt`

- [ ] **Step 1: Run both test suites**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Expected: both print `PASS` and exit 0.

- [ ] **Step 2: Validate existing-file patch**

Apply `patches/wireless_static_library_integration.patch` to the captured
baseline and require the resulting main project SHA-256 to match the integrated
project.

- [ ] **Step 3: Record handoff**

Because the workspace has no Git or SVN metadata, list the two new project
files, generator, tests, documentation changes, archive hashes, build outcome,
and customer-stage checks in the verification report. Do not create a commit or
claim a PR.
