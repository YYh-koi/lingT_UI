# ATBM Network Config Header Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the AP/P2P GO IPv4 server defaults into the ATBM public header and remove the ATBM implementation's dependency on 8189 `wlan_setting.h`.

**Architecture:** Keep the existing compile-time configuration model and `netif_set_addr()` flow. Define ATBM-prefixed IPv4 octet macros in both delivered copies of `atbm_wifi.h`, then consume only those macros from `atbm_wifi_interface.c`.

**Tech Stack:** C99, LwIP `netif`, PowerShell contract tests, Generalplus ARM GCC public-header syntax check.

---

### Task 1: Add a failing network-configuration contract

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [x] **Step 1: Extend the existing contract test**

After `$headerText` is loaded, load the delivered header copy and add exact checks for both headers:

```powershell
$deliveryHeaderPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\atbm_wifi.h'
if (-not (Test-Path -LiteralPath $deliveryHeaderPath -PathType Leaf)) {
    throw "Missing delivered WiFi interface header: $deliveryHeaderPath"
}
$deliveryHeaderText = Get-Content -LiteralPath $deliveryHeaderPath -Raw

$expectedNetworkMacros = [ordered]@{
    ATBM_WIFI_SERVER_IP_ADDR0 = 192
    ATBM_WIFI_SERVER_IP_ADDR1 = 168
    ATBM_WIFI_SERVER_IP_ADDR2 = 13
    ATBM_WIFI_SERVER_IP_ADDR3 = 1
    ATBM_WIFI_SERVER_NETMASK_ADDR0 = 255
    ATBM_WIFI_SERVER_NETMASK_ADDR1 = 255
    ATBM_WIFI_SERVER_NETMASK_ADDR2 = 255
    ATBM_WIFI_SERVER_NETMASK_ADDR3 = 0
    ATBM_WIFI_SERVER_GATEWAY_ADDR0 = 192
    ATBM_WIFI_SERVER_GATEWAY_ADDR1 = 168
    ATBM_WIFI_SERVER_GATEWAY_ADDR2 = 13
    ATBM_WIFI_SERVER_GATEWAY_ADDR3 = 1
}
foreach ($networkHeader in @($headerText, $deliveryHeaderText)) {
    foreach ($entry in $expectedNetworkMacros.GetEnumerator()) {
        Assert-Match `
            -Text $networkHeader `
            -Pattern ("#\s*define\s+{0}\s+{1}U?\b" -f [regex]::Escape($entry.Key), $entry.Value) `
            -Message "$($entry.Key) has ATBM default value $($entry.Value)"
    }
}
if ($headerText -cne $deliveryHeaderText) {
    $script:Failures++
    Write-Host 'FAIL: API and delivered atbm_wifi.h copies are identical' -ForegroundColor Red
}
```

After `$interfaceText` is loaded, add dependency and usage checks:

```powershell
Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '#\s*include\s*"wlan_setting\.h"' `
    -Message 'ATBM interface does not include 8189 wlan_setting.h'

foreach ($legacyMacro in @(
    'GP_AUTH_IP_ADDR',
    'NETMASK_ADDR',
    'GW_ADDR'
)) {
    Assert-NotMatch `
        -Text $interfaceText `
        -Pattern ("\b{0}[0-3]\b" -f $legacyMacro) `
        -Message "ATBM interface does not use legacy $legacyMacro macros"
}

foreach ($entry in $expectedNetworkMacros.GetEnumerator()) {
    Assert-Match `
        -Text $interfaceText `
        -Pattern ("\b{0}\b" -f [regex]::Escape($entry.Key)) `
        -Message "ATBM interface uses $($entry.Key)"
}
```

- [x] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: FAIL because the `ATBM_WIFI_SERVER_*` macros do not exist yet and `atbm_wifi_interface.c` still includes `wlan_setting.h` and uses the legacy macros.

### Task 2: Move network defaults to the ATBM public header

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- Test: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [x] **Step 1: Add identical ATBM macros to both public headers**

Place these definitions after the existing public size constants in both `atbm_wifi.h` copies:

```c
#define ATBM_WIFI_SERVER_IP_ADDR0        192U
#define ATBM_WIFI_SERVER_IP_ADDR1        168U
#define ATBM_WIFI_SERVER_IP_ADDR2         13U
#define ATBM_WIFI_SERVER_IP_ADDR3          1U

#define ATBM_WIFI_SERVER_NETMASK_ADDR0   255U
#define ATBM_WIFI_SERVER_NETMASK_ADDR1   255U
#define ATBM_WIFI_SERVER_NETMASK_ADDR2   255U
#define ATBM_WIFI_SERVER_NETMASK_ADDR3     0U

#define ATBM_WIFI_SERVER_GATEWAY_ADDR0   192U
#define ATBM_WIFI_SERVER_GATEWAY_ADDR1   168U
#define ATBM_WIFI_SERVER_GATEWAY_ADDR2    13U
#define ATBM_WIFI_SERVER_GATEWAY_ADDR3     1U
```

- [x] **Step 2: Remove the 8189 include and consume the ATBM macros**

Delete this include from `atbm_wifi_interface.c`:

```c
#include "wlan_setting.h"
```

Replace the three `IP4_ADDR()` argument groups in `atbm_wifi_network_server_start_locked()` with:

```c
IP4_ADDR(
    &ipaddr,
    ATBM_WIFI_SERVER_IP_ADDR0,
    ATBM_WIFI_SERVER_IP_ADDR1,
    ATBM_WIFI_SERVER_IP_ADDR2,
    ATBM_WIFI_SERVER_IP_ADDR3);
IP4_ADDR(
    &netmask,
    ATBM_WIFI_SERVER_NETMASK_ADDR0,
    ATBM_WIFI_SERVER_NETMASK_ADDR1,
    ATBM_WIFI_SERVER_NETMASK_ADDR2,
    ATBM_WIFI_SERVER_NETMASK_ADDR3);
IP4_ADDR(
    &gateway,
    ATBM_WIFI_SERVER_GATEWAY_ADDR0,
    ATBM_WIFI_SERVER_GATEWAY_ADDR1,
    ATBM_WIFI_SERVER_GATEWAY_ADDR2,
    ATBM_WIFI_SERVER_GATEWAY_ADDR3);
```

- [x] **Step 3: Run the focused test and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: `PASS: ATBM WiFi interface public contract` and exit code 0.

### Task 3: Verify public-header and P2P compatibility

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`

- [x] **Step 1: Compile-check the public header**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
```

Expected: `PASS: ATBM public API headers compile without private include paths` and exit code 0.

- [x] **Step 2: Run the P2P application contract**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1
```

Expected: PASS and exit code 0.

- [x] **Step 3: Run a final source audit**

Run:

```powershell
rg -n -S 'wlan_setting\.h|\bGP_AUTH_IP_ADDR[0-3]\b|\bNETMASK_ADDR[0-3]\b|\bGW_ADDR[0-3]\b' `
  gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c
```

Expected: no matches and exit code 1 from `rg` because the forbidden dependencies are absent.

Run:

```powershell
$api = Get-FileHash gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h
$delivery = Get-FileHash gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h
if ($api.Hash -ne $delivery.Hash) { throw 'atbm_wifi.h copies differ' }
```

Expected: exit code 0 with no exception.

### Task 4: Record the workspace limitation

**Files:**
- No production files changed in this task.

- [x] **Step 1: Confirm version-control availability**

Run:

```powershell
git rev-parse --show-toplevel
```

Expected in this workspace: FAIL with `not a git repository`; do not attempt a commit and report this limitation in the handoff.
