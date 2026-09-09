# ATBM WiFi Single-Header Delivery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `atbm_wifi.h` the only header needed with `libatbm_wifi.a` for customer WiFi integration.

**Architecture:** Merge the customer-facing types and declarations into one self-contained header while keeping raw ATBM headers private to the complete SDK. Build the same implementation into the independent WiFi archive, and change customer staging to expose only the new header for WiFi.

**Tech Stack:** C, Generalplus G+ project XML, ARM GCC 4.9.3, PowerShell packaging/tests.

---

### Task 1: Define the single-header contract

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_public_api.ps1`
- Modify: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [ ] **Step 1: Change the public-header compile test**

Replace the customer header list with:

```powershell
foreach ($header in @('atbm_wifi.h')) {
```

- [ ] **Step 2: Change the interface contract path and dependency rules**

Load `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`, retain checks for all
11 `atbm_wifi_*` functions and public types, and reject includes of:

```text
atbm_wifi_interface.h
atbm_wifi_types.h
atbm_hal.h
atbm_wifi_driver_api.h
atbm_api.h
lwip
FreeRTOS
SDIO
```

- [ ] **Step 3: Run the tests and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/tests/test_atbm_public_api.ps1
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: both fail because `atbm_wifi.h` does not exist.

### Task 2: Create and consume `atbm_wifi.h`

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Delete: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_interface.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- Modify: `Project/wifi_demo/atbm_wifi_probe_demo.c`

- [ ] **Step 1: Merge the public types and functions**

Create `atbm_wifi.h` with the basic ATBM integer aliases required by the API,
all current security/mode/state/error/config/scan/status/event types, and the
existing 11 function declarations. The header must use an
`ATBM_WIFI_H` include guard and `extern "C"` for C++.

- [ ] **Step 2: Migrate implementation and demo includes**

Replace:

```c
#include "atbm_wifi_interface.h"
```

with:

```c
#include "atbm_wifi.h"
```

- [ ] **Step 3: Remove the obsolete customer interface header**

Delete `atbm_wifi_interface.h`. Keep `atbm_wifi_types.h` only as an internal raw-driver dependency; customer staging must not copy it.

- [ ] **Step 4: Run the tests and verify GREEN**

Run the two commands from Task 1.

Expected:

```text
PASS: ATBM public API headers compile without private include paths
PASS: ATBM WiFi interface public contract
```

### Task 3: Restrict customer staging to one WiFi header

**Files:**
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `docs/wireless_static_library_integration.md`

- [ ] **Step 1: Write the staging assertions**

Change the minimal-stage fixture to create both public and private ATBM headers,
then assert that the staged `api` directory contains exactly:

```text
atbm_wifi.h
```

Assert that `atbm_wifi_types.h`, `atbm_wifi_driver_api.h`, and `atbm_api.h`
are absent.

- [ ] **Step 2: Run the tool test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: fail because staging currently copies every `api/*.h`.

- [ ] **Step 3: Restrict the staging function**

In `New-MinimalWirelessCustomerStage`, replace the wildcard ATBM header
collection with the exact file:

```powershell
$atbmPublicApiHeaders = @(
    Join-Path $atbmPublicApiRoot 'atbm_wifi.h'
)
```

Keep the existing check that rejects dependencies on private ATBM files.

- [ ] **Step 4: Update the customer integration document**

Document the WiFi handoff as:

```text
atbm_wifi.h
libatbm_wifi.a
```

State that the customer includes only `atbm_wifi.h`.

- [ ] **Step 5: Run the tool test and verify GREEN**

Expected:

```text
PASS: wireless library tool contract tests
```

### Task 4: Rebuild and verify the archive

**Files:**
- Modify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj` only if its source membership changes
- Output: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`

- [ ] **Step 1: Run all contract tests**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: four PASS results.

- [ ] **Step 2: Build the independent archives**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected: WiFi archive has 93 members and BTstack archive has 59 members.

- [ ] **Step 3: Check all WiFi API symbols**

Run `arm-none-eabi-nm -g --defined-only libatbm_wifi.a` and require:

```text
atbm_wifi_start_ap
atbm_wifi_stop_ap
atbm_wifi_start_sta
atbm_wifi_stop_sta
atbm_wifi_start_p2p
atbm_wifi_stop_p2p
atbm_wifi_scan
atbm_wifi_get_status
atbm_wifi_get_mac
atbm_wifi_register_event_callback
atbm_wifi_deinit
```

### Task 5: Regenerate the official customer package and patch

**Files:**
- Output: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`
- Create or replace: `patches/atbm_wifi_single_header.patch`

- [ ] **Step 1: Stage a new customer package**

Run `package_wireless_release.ps1` to a unique sibling staging directory.

- [ ] **Step 2: Verify the staged WiFi handoff**

Require:

```text
api/atbm_wifi.h
wlan_lib/libatbm_wifi.a
```

and require zero other `api/*.h` files and zero ATBM implementation `.c` files.

- [ ] **Step 3: Run the 309-object customer build**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/verify_minimal_customer_build.ps1 `
  -CustomerRoot <stage>
```

Expected: 309 compile units, 20 archives, successful link.

- [ ] **Step 4: Replace the official customer directory without retaining a backup**

After successful verification, delete the old official customer directory and
rename the verified staging directory to the official name.

- [ ] **Step 5: Generate and validate the patch**

Generate `patches/atbm_wifi_single_header.patch` from the captured pre-change
versions of every modified existing file. New files are delivered directly.
Run `git apply --check` against the captured baseline and compare patched files
byte-for-byte with the current versions.

- [ ] **Step 6: Final verification**

Re-run the four contract tests, checksum the official customer package, verify
the 11 archive symbols, and confirm the customer `api` directory contains only
`atbm_wifi.h`.

The workspace is not a Git repository, so commit steps are not applicable.
