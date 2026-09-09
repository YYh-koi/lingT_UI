# ATBM RTK Dependency Isolation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the ATBM WiFi library and ATBM main project build with the complete `gplib/liba_wifi_src/rtk` directory absent, while retaining a separate buildable 8189FTV project.

**Architecture:** Copy only the production lwIP 2.1.3 core, FreeRTOS port, and DHCP server into an ATBM-owned Generalplus network subtree. Replace the RTK `LwIP_Init()` dependency with a small ATBM TCP/IP-port adapter, keep ATBM's existing `netif_add()` implementation, and split the mixed main project into ATBM and 8189FTV project files.

**Tech Stack:** C, lwIP 2.1.3, FreeRTOS, G+ IDE `.gpcmprj`, ARM GCC, PowerShell contract tests.

---

### Task 1: Add a failing isolation contract

**Files:**
- Create: `tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1`

- [ ] **Step 1: Create the static dependency contract**

The test resolves the workspace, loads both ATBM project files, and checks the required boundaries:

```powershell
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$atbmLibraryProject = Join-Path $workspace 'Project\WirelessLibraries\ATBM_WiFi_Library\ATBM_WiFi_Library.gpcmprj'
$atbmMainProject = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj'
$rtkMainProject = Join-Path $workspace 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj'
$networkRoot = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\network'

foreach ($required in @(
    $atbmLibraryProject,
    $atbmMainProject,
    $rtkMainProject,
    (Join-Path $networkRoot 'include\lwipopts.h'),
    (Join-Path $networkRoot 'include\atbm_lwip_port.h'),
    (Join-Path $networkRoot 'port\atbm_lwip_port.c'),
    (Join-Path $networkRoot 'dhcp\dhcps.c'),
    (Join-Path $networkRoot 'dhcp\dhcps.h'),
    (Join-Path $networkRoot 'lwip_v2.1.3\ports\freertos\sys_arch.c'),
    (Join-Path $networkRoot 'lwip_v2.1.3\src\api\tcpip.c')
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing required ATBM isolation artifact: $required"
    }
}

$libraryText = Get-Content -LiteralPath $atbmLibraryProject -Raw
$mainText = Get-Content -LiteralPath $atbmMainProject -Raw
$rtkText = Get-Content -LiteralPath $rtkMainProject -Raw

foreach ($atbmText in @($libraryText, $mainText)) {
    if ($atbmText -match 'liba_wifi_src[/\\]rtk') {
        throw 'ATBM project still references liba_wifi_src/rtk'
    }
}
if ($mainText -notmatch 'WIFI_VENDOR=3') { throw 'ATBM main project must define WIFI_VENDOR=3' }
if ($mainText -notmatch 'libatbm_wifi\.a') { throw 'ATBM main project must link libatbm_wifi.a' }
if ($mainText -match 'lib8189FTV_WLAN\.a') { throw 'ATBM main project must not link lib8189FTV_WLAN.a' }
if ($rtkText -notmatch 'WIFI_VENDOR=2') { throw '8189FTV project must define WIFI_VENDOR=2' }
if ($rtkText -notmatch 'liba_wifi_src[/\\]rtk') { throw '8189FTV project must retain RTK sources' }
if ($rtkText -notmatch 'lib8189FTV_WLAN\.a') { throw '8189FTV project must link lib8189FTV_WLAN.a' }
if ($rtkText -match 'libatbm_wifi\.a') { throw '8189FTV project must not link libatbm_wifi.a' }

$interfacePath = Join-Path $workspace 'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_wifi_interface.c'
$interfaceText = Get-Content -LiteralPath $interfacePath -Raw
if ($interfaceText -match '#\s*include\s*["<]lwip_netconf\.h[">]') {
    throw 'ATBM interface still includes RTK lwip_netconf.h'
}
if ($interfaceText -notmatch '\batbm_lwip_system_init\s*\(') {
    throw 'ATBM interface does not use the ATBM lwIP port'
}

$dhcpsText = Get-Content -LiteralPath (Join-Path $networkRoot 'dhcp\dhcps.c') -Raw
if ($dhcpsText -match '#\s*include\s*["<]wlan_setting\.h[">]') {
    throw 'ATBM DHCP server still includes RTK wlan_setting.h'
}

Write-Host 'PASS: ATBM and RTK build dependencies are isolated'
```

- [ ] **Step 2: Run the contract and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1
```

Expected: FAIL because the ATBM-owned network files and separate 8189FTV project do not exist yet, and the ATBM project files still contain RTK paths.

### Task 2: Vendor the production TCP/IP stack into ATBM

**Files:**
- Create tree: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/lwip_v2.1.3/src/**`
- Create tree: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/lwip_v2.1.3/ports/freertos/**`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/include/lwipopts.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/dhcp/dhcps.c`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/dhcp/dhcps.h`

- [ ] **Step 1: Record and copy the exact production lwIP inputs**

Copy the lwIP `src` tree and FreeRTOS port without copying `ports/wifi`, which is the 8189 netif adapter:

```powershell
$source = Resolve-Path 'gplib\liba_wifi_src\rtk\network\lwip_v2.1.3'
$target = 'gplib\liba_wifi_src\atbm\atbm6165\platform\generalplus\network\lwip_v2.1.3'
New-Item -ItemType Directory -Force -Path $target | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'src') -Destination $target -Recurse
New-Item -ItemType Directory -Force -Path (Join-Path $target 'ports') | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'ports\freertos') -Destination (Join-Path $target 'ports') -Recurse
```

Then compare relative names and SHA-256 hashes for both copied trees. Expected: zero missing, extra, or hash-mismatched files.

- [ ] **Step 2: Copy and detach the lwIP configuration header**

Copy `rtk/8189FTV/project/generalplus/inc/lwipopts.h` to the ATBM `network/include` directory, preserve every existing lwIP option, and remove only:

```c
#include "autoconf.h"
```

Keep:

```c
#include "FreeRTOSConfig.h"
```

`CONFIG_DONT_CARE_TP` remains an optional externally supplied macro; no RTK header is needed for its `#ifdef`.

- [ ] **Step 3: Copy the production DHCP server and remove the unused RTK include**

Copy the current production `rtk/network/dhcp/dhcps.c` and `dhcps.h`, including the `[DHCPS]` tracing and current pool range. Remove this unused line from the copied `dhcps.c`:

```c
#include "wlan_setting.h"
```

Keep `project.h`, the current DHCP state machine, address-pool values, and trace code unchanged.

- [ ] **Step 4: Verify copied-stack integrity**

Run a SHA-256 comparison for the lwIP `src` and `ports/freertos` trees and run:

```powershell
rg -n -S 'liba_wifi_src[/\\]rtk|wlan_setting\.h|autoconf\.h' `
  gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network
```

Expected: no matches. The two intentionally modified files are verified by content rather than equal hashes.

### Task 3: Replace RTK network initialization with the ATBM port

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/include/atbm_lwip_port.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/port/atbm_lwip_port.c`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- Test: `tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1`

- [ ] **Step 1: Add the ATBM lwIP port interface**

Create `atbm_lwip_port.h`:

```c
#ifndef ATBM_LWIP_PORT_H
#define ATBM_LWIP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

void atbm_lwip_system_init(void);
void atbm_lwip_system_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
```

- [ ] **Step 2: Implement only TCP/IP core lifetime management**

Create `atbm_lwip_port.c`:

```c
#include <stddef.h>

#include "lwip/tcpip.h"

#include "atbm_lwip_port.h"

void atbm_lwip_system_init(void)
{
    tcpip_init(NULL, NULL);
}

void atbm_lwip_system_deinit(void)
{
    tcpip_deinit();
}
```

Do not create an `xnetif` array and do not call the RTK `wifi_ethernetif_init()`. ATBM continues to create each interface in `atbm_wifi_netif_init()`.

- [ ] **Step 3: Switch the ATBM public interface to the new port**

In `atbm_wifi_interface.c`, replace:

```c
#include "lwip_netconf.h"
```

with:

```c
#include "atbm_lwip_port.h"
```

Replace:

```c
LwIP_Init();
```

with:

```c
atbm_lwip_system_init();
```

Keep the existing `g_wifi.lwip_ready` once-only guard unchanged.

- [ ] **Step 4: Run the focused interface contracts**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
```

Expected: both tests pass; any old test that explicitly expects `LwIP_Init()` is updated to expect `atbm_lwip_system_init()` without weakening its other assertions.

### Task 4: Split and clean the two main-project configurations

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Create: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj`
- Generated: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk`
- Generated: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_8189FTV_Debug.mk`

- [ ] **Step 1: Preserve the current mixed project as the 8189FTV base**

Before editing the current project, copy it to `GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj`. In that copy:

- Replace every `-DWIFI_VENDOR=3` with `-DWIFI_VENDOR=2`.
- Change the project title to `GPCV2178_Turnkey_SDK_8189FTV`.
- Change Debug output/object directories to `prjDebug_8189FTV/bin/Debug/project` and `prjDebug_8189FTV/obj/Debug/`.
- Change Release output/object directories to `prjRelease_8189FTV/bin/Release/project` and `prjRelease_8189FTV/obj/Release/`.
- Remove ATBM WiFi include directories under `atbm6165/platform/generalplus/sdio`, `wlan_lib`, `api`, `include`, `os`, `hal`, and `net`.
- Remove `libatbm_wifi.a`.
- Retain `atbm6165/bt_api`, `libxstack_api.a`, and the BTstack paths because Bluetooth is independent of the WiFi vendor selection.

- [ ] **Step 2: Convert the formal project to ATBM-only include and link inputs**

In `GPCV2178_turnkey_sdk_release.gpcmprj`:

- Keep `-DWIFI_VENDOR=3`.
- Replace RTK network include paths with ATBM paths for `network`, `network/include`, `network/lwip_v2.1.3/src/include`, and `network/lwip_v2.1.3/ports/freertos/include`.
- Remove all `rtk/8189FTV` include paths.
- Remove `lib8189FTV_WLAN.a`.
- Keep `libatbm_wifi.a` and `libxstack_api.a`.

- [ ] **Step 3: Convert the formal project source units**

Apply these deterministic source-unit rules to the ATBM project:

1. Replace every `rtk/network/lwip_v2.1.3/src` prefix with the ATBM network `lwip_v2.1.3/src` prefix.
2. Replace every `rtk/network/lwip_v2.1.3/ports/freertos` prefix with the ATBM FreeRTOS-port prefix.
3. Replace `rtk/network/dhcp/dhcps.c/.h` with the ATBM DHCP files.
4. Remove `rtk/network/lwip_v2.1.3/ports/wifi/ethernetif.c/.h`.
5. Remove every remaining `rtk/8189FTV` unit, including RTK WiFi API, OS wrappers, SDIO, utilities and `lwip_netconf.c/.h`.
6. Add `platform/generalplus/network/port/atbm_lwip_port.c` with `<Option compilerVar="CC" />` and add its header unit.

After editing, parse both `.gpcmprj` files as XML to catch malformed nesting.

- [ ] **Step 4: Generate separate makefiles**

From `Project/GPCV2178_Turnkey_SDK`, run:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
& "$toolDir\gpcmprj2make.exe" --local -in 'GPCV2178_turnkey_sdk_release.gpcmprj' -targets Debug -out 'GPCV2178_turnkey_sdk_release_Debug.mk' -tp $toolDir -windows
& "$toolDir\gpcmprj2make.exe" --local -in 'GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj' -targets Debug -out 'GPCV2178_turnkey_sdk_release_8189FTV_Debug.mk' -tp $toolDir -windows
```

Expected: both commands exit 0. The ATBM makefile contains no `liba_wifi_src\rtk`; the 8189FTV makefile defines `WIFI_VENDOR=2` and retains RTK inputs.

### Task 5: Regenerate the ATBM WiFi library project without RTK includes

**Files:**
- Modify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Modify: `tools/wireless_lib/generate_wireless_projects.ps1`
- Generated: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library_Debug.mk`
- Test: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`

- [ ] **Step 1: Regenerate from the cleaned ATBM main project**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/generate_wireless_projects.ps1
```

Because the generator derives compile settings from the main project's Debug configuration, the regenerated ATBM library must inherit only the new ATBM network include paths. Update the generator's obsolete RTK exclusion entry so its own source contains no ATBM-to-RTK coupling.

- [ ] **Step 2: Verify the library project boundary**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1
```

Expected: both pass. `ATBM_WiFi_Library.gpcmprj` contains the ATBM network include paths and no RTK path.

- [ ] **Step 3: Generate the library makefile**

From `Project/WirelessLibraries/ATBM_WiFi_Library`, run:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
& "$toolDir\gpcmprj2make.exe" --local -in 'ATBM_WiFi_Library.gpcmprj' -targets Debug -out 'ATBM_WiFi_Library_Debug.mk' -tp $toolDir -windows
```

Expected: exit 0 and no `liba_wifi_src\rtk` occurrence in the generated makefile.

### Task 6: Build ATBM normally and run regression contracts

**Files:**
- Build output: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Build output: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Verify: existing `tools/wireless_lib/tests/*.ps1`

- [ ] **Step 1: Build the ATBM WiFi archive from scratch**

From `Project/WirelessLibraries/ATBM_WiFi_Library`, run:

```powershell
& 'D:\mingw32\bin\mingw32-make.exe' -f 'ATBM_WiFi_Library_Debug.mk' clean
& 'D:\mingw32\bin\mingw32-make.exe' -f 'ATBM_WiFi_Library_Debug.mk' -j4
```

Expected: exit 0 and a refreshed `libatbm_wifi.a`.

- [ ] **Step 2: Build the ATBM main project from scratch**

From `Project/GPCV2178_Turnkey_SDK`, run:

```powershell
& 'D:\mingw32\bin\mingw32-make.exe' -f 'GPCV2178_turnkey_sdk_release_Debug.mk' clean
& 'D:\mingw32\bin\mingw32-make.exe' -f 'GPCV2178_turnkey_sdk_release_Debug.mk' -j4
```

Expected: exit 0 and refreshed ELF/BIN outputs. Resolve compile or link failures by removing the specific remaining RTK compatibility dependency rather than adding RTK include directories back.

- [ ] **Step 3: Run the focused regression suite**

Run:

```powershell
$tests = @(
  'test_atbm_rtk_isolation.ps1',
  'test_atbm_wifi_interface.ps1',
  'test_atbm_public_api.ps1',
  'test_atbm_p2p_application_api.ps1',
  'test_atbm_generalplus_sdio_default.ps1',
  'test_atbm_sdio_host_mutex.ps1',
  'test_atbm_gp_priorities.ps1',
  'test_wireless_library_projects.ps1'
)
foreach ($test in $tests) {
  & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path 'tools\wireless_lib\tests' $test)
  if ($LASTEXITCODE -ne 0) { throw "$test failed" }
}
```

Expected: all tests pass.

### Task 7: Prove the ATBM build works with RTK absent

**Files:**
- Temporarily rename: `gplib/liba_wifi_src/rtk`
- Verify: ATBM library and main-project outputs

- [ ] **Step 1: Validate exact paths before the reversible rename**

Resolve both paths and require the source parent to equal the intended `gplib/liba_wifi_src` directory. Use the temporary name `rtk.__atbm_isolation_test__` and fail if it already exists.

- [ ] **Step 2: Rename, clean-build both ATBM targets, and always restore**

Use one PowerShell `try/finally` block:

```powershell
$wifiRoot = (Resolve-Path 'gplib\liba_wifi_src').Path
$rtkPath = Join-Path $wifiRoot 'rtk'
$heldPath = Join-Path $wifiRoot 'rtk.__atbm_isolation_test__'
if ((Split-Path -Parent $rtkPath) -ne $wifiRoot) { throw 'Unexpected RTK parent path' }
if (Test-Path -LiteralPath $heldPath) { throw "Temporary path already exists: $heldPath" }

Move-Item -LiteralPath $rtkPath -Destination $heldPath
try {
    Push-Location 'Project\WirelessLibraries\ATBM_WiFi_Library'
    try {
        & 'D:\mingw32\bin\mingw32-make.exe' -f 'ATBM_WiFi_Library_Debug.mk' clean
        if ($LASTEXITCODE -ne 0) { throw 'ATBM WiFi clean failed' }
        & 'D:\mingw32\bin\mingw32-make.exe' -f 'ATBM_WiFi_Library_Debug.mk' -j4
        if ($LASTEXITCODE -ne 0) { throw 'ATBM WiFi build failed' }
    } finally { Pop-Location }

    Push-Location 'Project\GPCV2178_Turnkey_SDK'
    try {
        & 'D:\mingw32\bin\mingw32-make.exe' -f 'GPCV2178_turnkey_sdk_release_Debug.mk' clean
        if ($LASTEXITCODE -ne 0) { throw 'ATBM main clean failed' }
        & 'D:\mingw32\bin\mingw32-make.exe' -f 'GPCV2178_turnkey_sdk_release_Debug.mk' -j4
        if ($LASTEXITCODE -ne 0) { throw 'ATBM main build failed' }
    } finally { Pop-Location }
} finally {
    if (Test-Path -LiteralPath $heldPath) {
        Move-Item -LiteralPath $heldPath -Destination $rtkPath
    }
}
```

Expected: both clean builds exit 0, and `rtk` is restored even if a command fails.

- [ ] **Step 3: Run the final artifact and reference audit**

Confirm the archive and ELF exist with current timestamps, then run:

```powershell
rg -n -S 'liba_wifi_src[/\\]rtk' `
  Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj `
  Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library_Debug.mk `
  Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj `
  Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk
```

Expected: no matches. Report that the 8189FTV configuration intentionally retains RTK references and was excluded from this ATBM-only audit.

### Task 8: Verify the retained 8189FTV configuration after restoration

**Files:**
- Verify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_8189FTV.gpcmprj`
- Verify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_8189FTV_Debug.mk`

- [ ] **Step 1: Re-run the static split contract with RTK restored**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1
```

Expected: PASS.

- [ ] **Step 2: Validate the 8189FTV generated build inputs**

Confirm the 8189FTV makefile has `WIFI_VENDOR=2`, links `lib8189FTV_WLAN.a`, retains the RTK lwIP/DHCP sources, does not link `libatbm_wifi.a`, and writes to the `_8189FTV` object/output directories.

- [ ] **Step 3: Build 8189FTV if the retained archive is present**

If `gplib/liba_wifi_src/rtk/8189FTV/wlan_lib/lib8189FTV_WLAN.a` exists, run:

```powershell
Push-Location 'Project\GPCV2178_Turnkey_SDK'
try {
    & 'D:\mingw32\bin\mingw32-make.exe' -f 'GPCV2178_turnkey_sdk_release_8189FTV_Debug.mk' -j4
    if ($LASTEXITCODE -ne 0) { throw '8189FTV retained build failed' }
} finally {
    Pop-Location
}
```

Expected: exit 0. If the prebuilt 8189 archive is absent, report that external input explicitly while preserving the validated 8189FTV project configuration.

### Task 9: Record the no-VCS workspace limitation

**Files:**
- No production files changed in this task.

- [ ] **Step 1: Confirm repository metadata remains unavailable**

Run:

```powershell
Get-ChildItem -Force -Name | Where-Object { $_ -eq '.git' -or $_ -eq '.svn' }
```

Expected in this workspace: no output. Do not fabricate commits; report changed files, build evidence, and remaining runtime-test scope directly.
