# ATBM Network Address Initialization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stop `atbm_wifi_network_server_start_locked()` from overriding the address assigned during netif initialization, while keeping both delivered `atbm_wifi.h` files as public application headers.

**Architecture:** `atbm_wifi_netif_init()` remains the single owner of the default `192.168.13.1/24` address through `netif_add()`. Network-server startup only brings the selected netif up and initializes the DHCP Server from the netif's current address, so a prior dynamic `netif_set_addr()` is preserved.

**Tech Stack:** C99, LwIP `netif`, PowerShell source-contract tests.

---

### Task 1: Replace the obsolete public-header network contract

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [x] **Step 1: Load the private netif initialization source**

After `$deliveryHeaderText` is loaded, add:

```powershell
$netifInitPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\atbm_os_skbuf.c'
if (-not (Test-Path -LiteralPath $netifInitPath -PathType Leaf)) {
    throw "Missing ATBM netif initialization source: $netifInitPath"
}
$netifInitText = Get-Content -LiteralPath $netifInitPath -Raw
```

- [x] **Step 2: Replace the server-macro assertions with ownership assertions**

Delete `$expectedNetworkMacros` and both loops that require those macros in the headers and interface source. Add the public-header and netif-initialization assertions immediately after `$netifInitText` is loaded:

```powershell
foreach ($networkHeader in @($headerText, $deliveryHeaderText)) {
    Assert-NotMatch `
        -Text $networkHeader `
        -Pattern '\bATBM_WIFI_SERVER_(?:IP|NETMASK|GATEWAY)_ADDR[0-3]\b' `
        -Message 'public atbm_wifi.h does not expose internal server address macros'
}

Assert-Match `
    -Text $netifInitText `
    -Pattern 'ucIPAddress\s*\[\s*4\s*\]\s*=\s*\{\s*192\s*,\s*168\s*,\s*13\s*,\s*1\s*\}' `
    -Message 'netif initialization owns default address 192.168.13.1'
Assert-Match `
    -Text $netifInitText `
    -Pattern 'ucNetMask\s*\[\s*4\s*\]\s*=\s*\{\s*255\s*,\s*255\s*,\s*255\s*,\s*0\s*\}' `
    -Message 'netif initialization owns default netmask 255.255.255.0'
Assert-Match `
    -Text $netifInitText `
    -Pattern 'netif_add\s*\(\s*p_netif\s*,\s*&ip_addr\s*,\s*&netmask\s*,\s*&gw' `
    -Message 'netif_add receives the default address during initialization'
```

After `$interfaceText = Get-Content -LiteralPath $interfacePath -Raw`, add:

```powershell
Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '\bnetif_set_addr\s*\(' `
    -Message 'network server startup does not override the initialized netif address'
Assert-NotMatch `
    -Text $interfaceText `
    -Pattern '\bATBM_WIFI_SERVER_' `
    -Message 'network server startup does not consume public address macros'

Assert-Match `
    -Text $interfaceText `
    -Pattern 'atbm_wifi_network_server_start_locked[\s\S]{0,2500}netif_set_default\s*\([\s\S]{0,300}netif_set_link_up\s*\([\s\S]{0,300}netif_set_up\s*\([\s\S]{0,500}dhcps_init\s*\(\s*netif\s*\)' `
    -Message 'network server startup still enables the netif and starts DHCP Server'
```

Keep the existing byte-equality check for the two delivered public headers.

- [x] **Step 3: Run the focused contract and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: FAIL because both public headers still expose `ATBM_WIFI_SERVER_*` and `atbm_wifi_network_server_start_locked()` still calls `netif_set_addr()`.

### Task 2: Make netif initialization the only default-address owner

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h:13`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h:13`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c:379`
- Test: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [x] **Step 1: Remove internal address macros from both public headers**

Delete the complete `ATBM_WIFI_SERVER_IP_ADDR*`, `ATBM_WIFI_SERVER_NETMASK_ADDR*`, and `ATBM_WIFI_SERVER_GATEWAY_ADDR*` definition blocks from both copies of `atbm_wifi.h`. Do not remove or rename either header and do not change the public declarations below the blocks.

- [x] **Step 2: Remove address overriding from network-server startup**

Change the function to the following minimal form:

```c
static int atbm_wifi_network_server_start_locked(unsigned int if_id)
{
    struct netif *netif = atbm_wifi_netif_get_vif((atbm_uint8)if_id);

    if(netif == NULL)
    {
        return ATBM_WIFI_ERR_NETWORK_FAILED;
    }

    netif_set_default(netif);
    netif_set_link_up(netif);
    netif_set_up(netif);

    if(g_wifi.dhcps_ready == ATBM_FALSE)
    {
        dhcps_init(netif);
        g_wifi.dhcps_ready = ATBM_TRUE;
    }

    return ATBM_WIFI_OK;
}
```

- [x] **Step 3: Run the focused contract and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: `PASS: ATBM WiFi interface public contract` and exit code 0.

### Task 3: Verify public API and P2P compatibility

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/os/freertos/atbm_os_skbuf.c`

- [x] **Step 1: Run public-header compilation checks**

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

- [x] **Step 3: Audit address ownership**

Run:

```powershell
rg -n -S 'ATBM_WIFI_SERVER_|netif_set_addr\s*\(' `
  gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h `
  gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h `
  gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c
```

Expected: no matches and exit code 1 from `rg`.

Run:

```powershell
rg -n -S 'ucIPAddress|ucNetMask|netif_add\s*\(' `
  gplib/liba_wifi_src/atbm/atbm6165/os/freertos/atbm_os_skbuf.c
```

Expected: matches for the `192.168.13.1/24` constants and their use by `netif_add()`.

### Task 4: Record version-control limitation

**Files:**
- No production files changed in this task.

- [x] **Step 1: Do not attempt commits in this workspace**

`git rev-parse --show-toplevel` has already returned `fatal: not a git repository`. Preserve all unrelated files and report the changed files without attempting a commit.
