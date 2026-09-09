# Wireless Delivery Layout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver the xStack archive and its single application header from `atbm6165/bt`, while delivering the ATBM WiFi archive and its single application header from `atbm6165/wlan_lib`.

**Architecture:** Keep original headers in the BTstack and ATBM source trees for internal compilation, and publish synchronized application-facing copies into the canonical customer directories. Update G+IDE projects, command-line archive publishing, minimal/broad customer staging, documentation, and tests so every active consumer uses the split layout.

**Tech Stack:** C, G+IDE XML projects, PowerShell build/package tooling, ARM GCC 4.9.3, PowerShell contract tests.

---

### Task 1: Lock the split-layout contract

**Files:**
- Modify: `btstack/adapter/tests/test_bt_xstack_build_integration.ps1`
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Modify: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`

- [ ] **Step 1: Change the xStack project assertions**

Require the library output to match `/atbm6165/bt/xstack`, require the main
project library path to end in `/atbm6165/bt/libxstack.a`, and reject
`wlan_lib/libxstack.a`. Require `bt_xstack_demo.c` to contain:

```c
#include "bt_xstack.h"
```

- [ ] **Step 2: Change the wireless delivery assertions**

Use this exact fixture layout and exact published-header counts:

```text
gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a
gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h
```

Assert that `api/atbm_wifi.h`, `wlan_lib/libxstack.a`, and additional headers
are absent from staged output. Change the BTstack G+ project expected output
to `../../../gplib/liba_wifi_src/atbm/atbm6165/bt/xstack`.

- [ ] **Step 3: Run the four focused tests and verify RED**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/adapter/tests/test_bt_xstack_build_integration.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: failures identify the old xStack archive location, missing delivery
headers, and old staging rules.

### Task 2: Publish application headers and update G+IDE integration

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `Project/wifi_demo/bt_xstack_demo.c`

- [ ] **Step 1: Create exact public-header copies**

Copy the complete contents of:

```text
btstack/include/adapter/bt_xstack.h
gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h
```

to their canonical `bt` and `wlan_lib` destinations. Do not remove or edit
the source headers.

- [ ] **Step 2: Move the xStack G+IDE output and main link**

Use these exact XML values:

```xml
<Option output="../../../gplib/liba_wifi_src/atbm/atbm6165/bt/xstack" prefix_auto="1" extension_auto="1" />
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a" />
```

Add the following Debug compiler include directories before the source-tree
ATBM/BTstack include directories:

```xml
<Add directory="../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib" />
<Add directory="../../gplib/liba_wifi_src/atbm/atbm6165/bt" />
```

- [ ] **Step 3: Switch the demo to the delivery header**

Replace:

```c
#include "adapter/bt_xstack.h"
```

with:

```c
#include "bt_xstack.h"
```

- [ ] **Step 4: Run the project/demo tests and verify GREEN**

Run the first three commands from Task 1. Expected: PASS.

### Task 3: Split command-line archive publishing

**Files:**
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `tools/wireless_lib/build_wireless_libs.ps1`
- Modify: `tools/wireless_lib/generate_wireless_projects.ps1`

- [ ] **Step 1: Give archive builds a canonical wireless root**

Rename `CustomerLibraryDirectory` to `CustomerWirelessRoot`. In
`Invoke-WirelessArchiveBuild`, derive:

```powershell
$wifiDirectory = Join-Path $customerRoot 'wlan_lib'
$bluetoothDirectory = Join-Path $customerRoot 'bt'
```

Describe archives with their destination and public header:

```powershell
@{
    Name = 'libatbm_wifi.a'
    DeliveryDirectory = $wifiDirectory
    HeaderSource = Join-Path $Plan.WorkspaceRoot 'gplib\liba_wifi_src\atbm\atbm6165\api\atbm_wifi.h'
    HeaderName = 'atbm_wifi.h'
}
@{
    Name = 'libxstack.a'
    DeliveryDirectory = $bluetoothDirectory
    HeaderSource = Join-Path $Plan.WorkspaceRoot 'btstack\include\adapter\bt_xstack.h'
    HeaderName = 'bt_xstack.h'
}
```

Copy each header after publishing the stripped archive. The returned
`CustomerPath` must identify the archive in its split destination.

- [ ] **Step 2: Update the CLI default and generator**

Set the build script default to:

```powershell
CustomerWirelessRoot = <workspace>\gplib\liba_wifi_src\atbm\atbm6165
```

Generate the BTstack project with output
`../../../gplib/liba_wifi_src/atbm/atbm6165/bt/xstack`; keep WiFi output in
`wlan_lib/atbm_wifi`.

- [ ] **Step 3: Run the wireless tool/project tests**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: PASS.

### Task 4: Update customer staging and delivery documentation

**Files:**
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `docs/wireless_static_library_integration.md`
- Modify: `patches/wireless_static_library_integration.patch`

- [ ] **Step 1: Restrict minimal staging to four wireless files**

Treat these as the only allowed ATBM customer files:

```powershell
$wirelessDeliverables = @(
    'gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a',
    'gplib\liba_wifi_src\atbm\atbm6165\bt\bt_xstack.h',
    'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a',
    'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h'
)
```

Reject other dependencies below `atbm6165`, and strip/reindex the two archive
paths individually.

- [ ] **Step 2: Align broad staging**

Stop recursively copying wireless headers. Copy the exact four deliverables,
then strip/reindex `wlan_lib/libatbm_wifi.a` and `bt/libxstack.a`.

- [ ] **Step 3: Update docs and active delivery patch**

Document this tree and the corresponding two linker paths:

```text
atbm6165/bt/{libxstack.a,bt_xstack.h}
atbm6165/wlan_lib/{libatbm_wifi.a,atbm_wifi.h}
```

- [ ] **Step 4: Run packaging tests and verify GREEN**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: PASS with exactly two public wireless headers in the fixture stage.

### Task 5: Rebuild, package, and verify

**Files:**
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Generated: `output/customer_wireless_sdk`

- [ ] **Step 1: Build both wireless archives**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected: 96 WiFi and 60 Bluetooth objects, with archives and headers in the
split delivery directories.

- [ ] **Step 2: Remove only the obsolete xStack archive**

After the new archive exists, remove exactly
`gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libxstack.a`. Preserve
`wlan_lib/libatbm_wifi.a` and both public headers.

- [ ] **Step 3: Generate and build the main Debug project**

Generate the makefile using `gpcmprj2make.exe --local` from
`Project/GPCV2178_Turnkey_SDK`, then run the bundled `make.exe debug` with the
G+IDE toolchain first on `PATH`. Expected: exit 0.

- [ ] **Step 4: Verify symbols**

```powershell
arm-none-eabi-nm.exe -g --defined-only gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a
arm-none-eabi-nm.exe -g --defined-only Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf
```

Expected archive symbols: `bt_xstack_init`, `bt_xstack_send_at`. Expected ELF
symbols: those two plus `bt_xstack_demo_start`.

- [ ] **Step 5: Run all relevant PowerShell tests**

Run every `.ps1` under `btstack`, `Project/wifi_demo/tests`, and
`tools/wireless_lib/tests` in an independent PowerShell process. Expected:
20 tests, 0 failures.

- [ ] **Step 6: Regenerate and inspect the customer package**

Move the existing `output/customer_wireless_sdk` to a recoverable timestamped
backup, run `package_wireless_release.ps1` with the original stage path, and
verify its ATBM subtree contains exactly the four specified deliverables and
no `wlan_lib/libxstack.a`.

This workspace is not a Git repository, so commit steps are intentionally
omitted; no branch or worktree cleanup is performed.
