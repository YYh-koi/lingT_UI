# Wireless Static Libraries Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and integrate separate ATBM WiFi and BTstack static libraries, then generate a customer SDK package containing wireless headers and `.a` files but no wireless C sources.

**Architecture:** A PowerShell module parses the approved source manifest and the main project's Debug compiler configuration, invokes the installed Generalplus GCC 4.9.3 tools, and produces deterministic archives. Thin build and packaging entry scripts call the module. The existing main project is patched to replace 149 wireless source units with two library inputs, and a staging builder copies the rest of the SDK while exposing only wireless headers and libraries.

**Tech Stack:** PowerShell 5, Generalplus ARM Embedded GCC 4.9.3, GNU `ar`/`ranlib`/`strip`/`nm`, G+ IDE `gpcmprj2make`, GNU Make, XML, JSON, unified patches.

---

## File Map

- Create `tools/wireless_lib/wireless_sources.json`: approved 92-file WiFi and 57-file BTstack source manifest.
- Create `tools/wireless_lib/WirelessLibTools.psm1`: configuration parsing, source validation, compilation, archiving, patch generation, staging, and artifact validation.
- Create `tools/wireless_lib/build_wireless_libs.ps1`: command-line library build entry point.
- Create `tools/wireless_lib/package_wireless_release.ps1`: command-line customer package entry point.
- Create `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`: dependency-free PowerShell unit/contract tests.
- Modify `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`: remove wireless C units and add the two archives to the Debug linker.
- Create `patches/wireless_static_library_integration.patch`: unified patch for the existing project file.
- Create `docs/wireless_static_library_integration.md`: build and customer integration instructions.
- Create `lib/libatbm_wifi.a` and `lib/libbtstack.a`: stripped customer link libraries.
- Create `output/wireless_libs/internal/*.a`: unstripped internal diagnostic archives.
- Create `output/customer_wireless_sdk/`: staged customer SDK and checksum manifest.

### Task 1: Source manifest and project configuration parser

**Files:**
- Create: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Create: `tools/wireless_lib/wireless_sources.json`
- Create: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] **Step 1: Write the failing manifest/parser tests**

The test imports `WirelessLibTools.psm1`, calls `Read-WirelessSourceManifest`
and `Get-DebugCompilerConfiguration`, and asserts:

```powershell
$manifest = Read-WirelessSourceManifest -Path $manifestPath -WorkspaceRoot $workspace
Assert-Equal 92 $manifest.Wifi.Count 'WiFi source count'
Assert-Equal 57 $manifest.Bluetooth.Count 'BTstack source count'
Assert-True (($manifest.Wifi + $manifest.Bluetooth | Where-Object { -not (Test-Path $_) }).Count -eq 0) 'all sources exist'

$config = Get-DebugCompilerConfiguration -ProjectFile $projectPath
Assert-Contains '-O2' $config.Options 'optimization'
Assert-Contains '-mcpu=cortex-a7' $config.Options 'CPU ABI'
Assert-Contains '-mfloat-abi=softfp' $config.Options 'float ABI'
Assert-Contains '-DWIFI_VENDOR=3' $config.Options 'WiFi vendor'
```

- [ ] **Step 2: Run the test and confirm RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: failure because `WirelessLibTools.psm1` and its functions do not
exist.

- [ ] **Step 3: Add the exact source manifest**

Extract the 149 wireless `<Unit filename="...c">` entries from the current
project before modifying it. Normalize each entry to a workspace-relative path,
split it into `wifi` and `bluetooth` arrays, and store:

```json
{
  "schema_version": 1,
  "project": "Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj",
  "configuration": "Debug",
  "expected_counts": {
    "wifi": 92,
    "bluetooth": 57
  },
  "wifi": [],
  "bluetooth": []
}
```

The arrays must contain every extracted path; empty arrays are only the schema
shape shown here and are not acceptable in the implemented file.

- [ ] **Step 4: Implement manifest and Debug configuration parsing**

`Read-WirelessSourceManifest` must reject a missing file, duplicate source,
wrong extension, missing source, or count mismatch. `Get-DebugCompilerConfiguration`
must parse the XML Debug target, return compiler `<Add option>` values and
resolved include directories, and reject any missing ABI option.

- [ ] **Step 5: Run the test and confirm GREEN**

Run the Task 1 test command. Expected: all manifest and compiler configuration
assertions pass with a final `PASS` line and exit code 0.

### Task 2: Reproducible library builder

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Create: `tools/wireless_lib/build_wireless_libs.ps1`

- [ ] **Step 1: Add failing build-plan tests**

Add a dry-run test for `New-WirelessCompilePlan` that asserts the selected
compiler is:

```text
D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe
```

and that its `--version` output contains `4.9.3`. Assert 92 WiFi compile actions,
57 Bluetooth compile actions, unique object paths, and the required ABI flags.

- [ ] **Step 2: Run the test and confirm RED**

Expected: failure because compile-plan/build functions and the entry script are
missing.

- [ ] **Step 3: Implement compile planning and process execution**

Implement:

```powershell
New-WirelessCompilePlan
Invoke-CheckedProcess
Invoke-WirelessArchiveBuild
```

Each source compiles with the parsed Debug options and includes into a dedicated
mirrored object path under `output/wireless_libs/obj`. Use the 4.9.3 compiler
and matching `arm-none-eabi-ar.exe`, `arm-none-eabi-ranlib.exe`,
`arm-none-eabi-strip.exe`, and `arm-none-eabi-nm.exe`.

- [ ] **Step 4: Implement the build entry script**

`build_wireless_libs.ps1` accepts workspace, project, manifest, toolchain, and
output parameters; defaults resolve relative to the script location. It builds
unstripped internal archives, copies them to root `lib`, strips only debug
sections from the customer copies, reindexes them, and exits nonzero on any
failure.

- [ ] **Step 5: Run unit tests and confirm GREEN**

Expected: dry-run compile-plan tests pass.

- [ ] **Step 6: Build both archives**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected: 92 WiFi and 57 BTstack sources compile successfully; both internal
and stripped customer archives are created.

- [ ] **Step 7: Audit archive structure**

Run `arm-none-eabi-ar.exe t`, `arm-none-eabi-nm.exe -s`, and
`arm-none-eabi-readelf.exe -p .comment` against representative extracted
members. Expected: readable archive indexes, required exported symbols, ARM ELF
objects, and GCC 4.9.3 compiler comments in internal objects.

### Task 3: Main project library integration and patch

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Create: `patches/wireless_static_library_integration.patch`

- [ ] **Step 1: Capture the original project outside the deliverable tree**

Copy the project file to
`output/wireless_libs/baseline/Project/GPCV2178_Turnkey_SDK/` and record its
SHA-256 before editing.

- [ ] **Step 2: Add failing integration tests**

Assert that the project has zero wireless C `<Unit>` entries and that the Debug
linker contains exactly:

```xml
<Add library="..\..\lib\libatbm_wifi.a" />
<Add library="..\..\lib\libbtstack.a" />
```

The test must fail against the unmodified project.

- [ ] **Step 3: Modify the project**

Remove only the 149 manifest-listed wireless C unit blocks. Preserve wireless
include directories and all unrelated units/options. Add the two library
entries to the Debug linker after existing SDK libraries.

- [ ] **Step 4: Run integration tests and confirm GREEN**

Expected: zero direct wireless source units, two library inputs, and unchanged
unrelated Debug compiler options.

- [ ] **Step 5: Generate the unified patch**

Generate a workspace-relative patch between the captured original and modified
project. Patch headers must be:

```text
--- a/Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj
+++ b/Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj
```

- [ ] **Step 6: Validate the patch**

Apply the patch to a fresh copy of the baseline project in a temporary
directory, compare its SHA-256 with the modified project, and require equality.

### Task 4: Full Debug integration build

**Files:**
- Generated: `output/wireless_libs/build/GPCV2178_Debug.mk`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Generated: corresponding map/binary files

- [ ] **Step 1: Generate the Debug makefile**

Use `gpcmprj2make.exe` with the modified project, `-cfg Debug`, Windows output,
and the installed 4.9.3 toolchain path.

- [ ] **Step 2: Perform a clean Debug build**

Use the G+ IDE bundled Make executable and capture the complete log under
`output/wireless_libs/logs`.

- [ ] **Step 3: Verify the final link**

Require exit code 0. Inspect the final ELF/map to verify WiFi and BTstack entry
points are defined and archive members from both libraries are linked. Confirm
the project does not compile any manifest-listed wireless C source.

### Task 5: Customer SDK staging

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Create: `tools/wireless_lib/package_wireless_release.ps1`
- Create: `docs/wireless_static_library_integration.md`
- Create: `output/customer_wireless_sdk/`

- [ ] **Step 1: Add failing staging tests**

Test `New-WirelessCustomerStage` against a temporary directory. Assert that:

```powershell
Assert-True (Test-Path "$stage/lib/libatbm_wifi.a") 'WiFi library staged'
Assert-True (Test-Path "$stage/lib/libbtstack.a") 'BTstack library staged'
Assert-Equal 0 @(Get-ChildItem $stage -Recurse -File -Filter *.c |
    Where-Object { $_.FullName -match 'atbm|btstack' }).Count 'no wireless C sources'
Assert-True (Test-Path "$stage/patches/wireless_static_library_integration.patch") 'patch staged'
```

- [ ] **Step 2: Run tests and confirm RED**

Expected: failure because customer staging functions and entry script are
missing.

- [ ] **Step 3: Implement staging**

Copy the SDK into `output/customer_wireless_sdk` while excluding generated
output, temp data, prior object trees, backups, tests, code-graph data, and the
duplicate top-level `atbm_rtos_wifi6` source tree. For
`gplib/liba_wifi_src/atbm/atbm6165` and `btstack`, copy header files needed by
the existing include paths but no `.c` files. Copy both libraries, the patched
project, patch, integration guide, and design/plan documentation.

- [ ] **Step 4: Generate manifest and checksums**

Create a sorted `SHA256SUMS.txt` containing each delivered file's
workspace-relative path, byte length, and SHA-256 hash. Re-read and verify every
entry.

- [ ] **Step 5: Run staging tests and confirm GREEN**

Expected: no wireless C files, both archives and all required integration
artifacts present, checksum verification succeeds.

### Task 6: Customer-form build and final verification

**Files:**
- Generated: `output/customer_wireless_sdk/build/GPCV2178_Debug.mk`
- Generated: `output/customer_wireless_sdk/build.log`

- [ ] **Step 1: Generate a Debug makefile inside the staged SDK**

Run the same `gpcmprj2make.exe` command against the staged project.

- [ ] **Step 2: Build the staged SDK**

Build with the bundled Make tool and GCC 4.9.3. Expected: compile and link exit
code 0 without access to wireless C sources.

- [ ] **Step 3: Run the complete contract test suite**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: all assertions pass and exit code 0.

- [ ] **Step 4: Run final artifact audits**

Verify archive indexes, required symbols, package source exclusion, patch
application, build logs, file sizes, and SHA-256 checksums. Record the exact
commands and outcomes in `output/wireless_libs/verification-report.txt`.

- [ ] **Step 5: Record the no-VCS checkpoint**

Because the workspace is neither a Git repository nor an SVN working copy,
record modified existing files, new files, artifact hashes, and verification
results in the delivery manifest instead of creating commits.
