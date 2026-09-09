# Sibling Customer SDK Package Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate a directly compressible customer SDK beside the complete internal SDK, containing wireless headers and archives but no ATBM WiFi or BTstack implementation source.

**Architecture:** Extend the existing tested staging function to exclude internal wireless library projects, change the packaging entry point's default to a sibling `_customer` directory, then regenerate and audit the complete package. The internal SDK remains the source-of-truth and is not moved or deleted.

**Tech Stack:** PowerShell 5, robocopy, SHA-256, XML contract tests, Generalplus GCC integration artifacts.

---

### Task 1: Customer-stage exclusion contract

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] **Step 1: Add a failing fixture assertion**

Create `Project\WirelessLibraries\ATBM_WiFi_Library` in the existing staging
fixture, place a dummy `.gpcmprj` in it, run `New-WirelessCustomerStage`, and
assert:

```powershell
Assert-True -Condition (-not (Test-Path -LiteralPath (
    Join-Path $stageRoot 'Project\WirelessLibraries'
))) -Message 'internal wireless library projects excluded'
```

- [ ] **Step 2: Run the test and confirm RED**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: only the new internal-project exclusion assertion fails.

- [ ] **Step 3: Implement the exclusion**

Add this absolute directory to `$excludedDirectories`:

```powershell
(Join-Path $workspace 'Project\WirelessLibraries')
```

- [ ] **Step 4: Run the test and confirm GREEN**

Expected: `PASS: wireless library tool contract tests`.

### Task 2: Sibling default destination

**Files:**
- Modify: `tools/wireless_lib/package_wireless_release.ps1`
- Modify: `docs/wireless_static_library_integration.md`

- [ ] **Step 1: Change the default destination**

When `-StageRoot` is omitted, derive:

```powershell
$workspaceParent = Split-Path -Parent $WorkspaceRoot
$workspaceName = Split-Path -Leaf $WorkspaceRoot
$StageRoot = Join-Path $workspaceParent ($workspaceName + '_customer')
```

Keep explicit `-StageRoot` behavior unchanged.

- [ ] **Step 2: Update the integration instructions**

State that the primary package output is:

```text
D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer
```

Explain that this whole directory may be compressed, and that internal wireless
projects and wireless C files are excluded.

### Task 3: Generate and verify the sibling package

**Files:**
- Generate: `D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`
- Update: `output/wireless_libs/verification-report.txt`

- [ ] **Step 1: Confirm the exact destination is absent or empty**

Use `Test-Path` and `Get-ChildItem -Force`. Do not remove or overwrite a
non-empty directory.

- [ ] **Step 2: Run the packager**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/package_wireless_release.ps1
```

Expected: the reported `Customer SDK staged` path ends in `_customer`.

- [ ] **Step 3: Audit source exclusions and required files**

Require:

```text
lib/libatbm_wifi.a: present
lib/libbtstack.a: present
gplib/.../atbm6165/**/*.c: 0
btstack/**/*.c: 0
Project/WirelessLibraries: absent
atbm_rtos_wifi6: absent
```

- [ ] **Step 4: Verify checksums**

Parse every `SHA256SUMS.txt` line and recompute byte length and SHA-256. Require
zero missing files, length mismatches, or hash mismatches.

- [ ] **Step 5: Run both contract suites**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Expected: both exit 0 and print `PASS`.
