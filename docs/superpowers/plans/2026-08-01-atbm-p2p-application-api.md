# ATBM P2P and Application API Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the GP ATBM6165 P2P GO lifecycle and expose reliable connection, IP, and AP/P2P client-management APIs through the existing self-contained `atbm_wifi.h` interface.

**Architecture:** Keep the customer ABI in `atbm_wifi.h` independent of ATBM, LwIP, SDIO, and RTOS headers. Add safe VIF-aware primitives to the private driver API, then implement public mode/state validation and type conversion in `atbm_wifi_interface.c`. Preserve the GP SDIO and LwIP/DHCP paths while matching the proven ATM630 STA/P2P hot-switch order without reloading firmware.

**Tech Stack:** C99, ATBM6165 WiFi driver, Generalplus GPCV2178 SDIO, FreeRTOS/CMSIS-RTOS, LwIP, PowerShell contract tests, ARM GCC static-library build.

---

### Task 1: Add failing public and P2P safety contract tests

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- Modify: `tools/wireless_lib/tests/test_atbm_public_api.ps1`
- Create: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

- [ ] **Step 1: Extend the public header contract with the new types and functions**

Add these type names to the existing `$typeName` loop in
`test_atbm_wifi_interface.ps1`:

```powershell
'atbm_wifi_connection_info_t',
'atbm_wifi_ip_info_t',
'atbm_wifi_client_info_t'
```

Add these function names to its function loop:

```powershell
'atbm_wifi_is_connected',
'atbm_wifi_get_connection_info',
'atbm_wifi_get_rssi',
'atbm_wifi_get_ip_info',
'atbm_wifi_get_clients',
'atbm_wifi_disconnect_client'
```

- [ ] **Step 2: Extend the standalone-header compile probe**

In the generated probe body in `test_atbm_public_api.ps1`, add declarations that
force the compiler to resolve every new public type without private include paths:

```c
static atbm_wifi_connection_info_t g_connection_info;
static atbm_wifi_ip_info_t g_ip_info;
static atbm_wifi_client_info_t g_client_info;
int atbm_public_header_probe(void)
{
    return (int)sizeof(g_connection_info) +
        (int)sizeof(g_ip_info) +
        (int)sizeof(g_client_info);
}
```

- [ ] **Step 3: Create a focused P2P/private-driver contract test**

Create `test_atbm_p2p_application_api.ps1` using the assertion helpers from
`test_atbm_wifi_interface.ps1`. Load `atbm_wifi_driver_api.h`,
`atbm_wifi_driver_api.c`, and `atbm_wifi_interface.c`, then require:

```powershell
$requiredPrivateFunctions = @(
    'atbm_wifi_get_associated_client_list_vif',
    'atbm_wifi_get_client_rssi_vif',
    'atbm_wifi_disconnect_client_vif',
    'atbm_wifi_p2p_go_stop'
)

foreach ($name in $requiredPrivateFunctions) {
    Assert-Match -Text $driverHeader -Pattern ("\b{0}\s*\(" -f $name) `
        -Message "$name is declared by the private driver API"
}

Assert-Match -Text $driverSource `
    -Pattern 'if\s*\(priv\s*==\s*ATBM_NULL\)[\s\S]{0,180}g_hw_prv\.p2p_if_id\s*=\s*priv->if_id' `
    -Message 'P2P start validates priv before dereference'
Assert-NotMatch -Text $driverSource `
    -Pattern 'g_hw_prv\.p2p_if_id\s*=\s*priv->if_id;[\s\S]{0,180}if\s*\(priv\s*==\s*ATBM_NULL\)' `
    -Message 'P2P start has no pre-check dereference'
Assert-Match -Text $driverSource `
    -Pattern 'ret\s*=\s*atbm_p2p_go_start\s*\(priv\)' `
    -Message 'P2P GO start propagates the lower-layer result'
Assert-Match -Text $driverSource `
    -Pattern 'capacity\s*=\s*maclist->count;[\s\S]{0,300}maclist->count\s*=\s*capacity' `
    -Message 'client-list capacity survives output clearing'
Assert-Match -Text $driverHeader `
    -Pattern 'pwd\s*\[\s*ATBM_P2P_PASSWORD_MAX_LEN\s*\+\s*1\s*\]' `
    -Message 'P2P password storage accepts 63 bytes plus terminator'
```

Also assert the six new public implementation symbols are present in
`atbm_wifi_interface.c`. Protect the already-required P2P event mapping with:

```powershell
Assert-Match -Text $interfaceSource `
    -Pattern 'ATBM_WIFI_ASSOCRSP_TXOK_EVENT[\s\S]{0,160}ATBM_WIFI_EVENT_CLIENT_CONNECTED' `
    -Message 'AP/P2P client-connect event is forwarded'
Assert-Match -Text $interfaceSource `
    -Pattern 'ATBM_WIFI_MODE_P2P_GO[\s\S]{0,180}ATBM_WIFI_EVENT_CLIENT_DISCONNECTED' `
    -Message 'P2P GO client-disconnect event is forwarded'
```

- [ ] **Step 4: Run the three tests and verify they fail for the intended gaps**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1
```

Expected: the first test reports missing new public declarations, the standalone
compile probe fails on the new types, and the P2P test reports the missing private
wrappers plus the current null-dereference/config-size/client-count defects.

### Task 2: Define the extended public and private contracts

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_driver_api.h`
- Test: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_public_api.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

- [ ] **Step 1: Add the three self-contained public result structures**

Place these after `atbm_wifi_status_t` in `atbm_wifi.h`:

```c
typedef struct {
    char ssid[ATBM_WIFI_SSID_MAX_LEN + 1U];
    unsigned char bssid[ATBM_WIFI_MAC_ADDR_LEN];
    int channel;
    int rssi;
    int phy_rate;
} atbm_wifi_connection_info_t;

typedef struct {
    unsigned char ip[4];
    unsigned char netmask[4];
    unsigned char gateway[4];
} atbm_wifi_ip_info_t;

typedef struct {
    unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN];
    int rssi;
} atbm_wifi_client_info_t;
```

- [ ] **Step 2: Add the six public function declarations**

Place these after `atbm_wifi_get_mac()`:

```c
int atbm_wifi_is_connected(int *connected);
int atbm_wifi_get_connection_info(atbm_wifi_connection_info_t *info);
int atbm_wifi_get_rssi(int *rssi);
int atbm_wifi_get_ip_info(atbm_wifi_ip_info_t *info);

int atbm_wifi_get_clients(atbm_wifi_client_info_t *clients,
    unsigned int max_clients,
    unsigned int *client_count);
int atbm_wifi_disconnect_client(
    const unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN]);
```

Do not add LwIP or private ATBM includes to the public header.

- [ ] **Step 3: Make the private P2P configuration sizes explicit and safe**

Replace the shared 32-byte array size in `atbm_wifi_driver_api.h` with:

```c
#define ATBM_P2P_DEVICE_NAME_MAX_LEN 32
#define ATBM_P2P_SSID_MAX_LEN        32
#define ATBM_P2P_PASSWORD_MAX_LEN    63

struct user_p2p_config {
    atbm_uint8 device_name[ATBM_P2P_DEVICE_NAME_MAX_LEN + 1];
    atbm_size_t device_name_len;
    atbm_uint8 ssid[ATBM_P2P_SSID_MAX_LEN + 1];
    atbm_size_t ssid_len;
    atbm_uint8 pwd[ATBM_P2P_PASSWORD_MAX_LEN + 1];
    atbm_size_t pwd_len;
    atbm_uint8 channel;
};
```

- [ ] **Step 4: Declare the VIF-aware private helpers**

Add:

```c
atbm_int32 atbm_wifi_get_associated_client_list_vif(
    atbm_uint8 if_id, atbm_void *buffer, unsigned int buffer_size);
int atbm_wifi_get_client_rssi_vif(
    atbm_uint8 if_id, const unsigned char *mac, int *rssi);
int atbm_wifi_disconnect_client_vif(
    atbm_uint8 if_id, const unsigned char *mac);
int atbm_wifi_p2p_go_stop(void);
```

Keep the existing legacy wrappers declared for internal callers.

- [ ] **Step 5: Run the public tests**

Run the first two Task 1 commands.

Expected: both PASS; the P2P-focused test still fails because implementation
changes have not yet been made.

### Task 3: Fix the private P2P lifecycle and client primitives

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_driver_api.c`
- Test: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

- [ ] **Step 1: Fix P2P start and GO result propagation**

Implement `atbm_wifi_p2p_start()` in this order:

```c
int atbm_wifi_p2p_start(void)
{
    struct atbmwifi_vif *priv = p2p_get_available_ifpriv();

    if(priv == ATBM_NULL)
    {
        wifi_printk(WIFI_ALWAYS, "p2p:error, priv is NULL\n");
        return -1;
    }

    g_hw_prv.p2p_if_id = priv->if_id;
    return atbm_p2p_start(priv);
}
```

Change the final call in `atbm_wifi_p2p_go_start()` to:

```c
ret = atbm_p2p_go_start(priv);
```

and return `ret`.

- [ ] **Step 2: Make repeated P2P user configuration bounded and leak-free**

Add a local bounded-length helper that never reads beyond the maximum plus one:

```c
static atbm_size_t atbm_wifi_p2p_bounded_strlen(
    const char *text, atbm_size_t limit)
{
    atbm_size_t length;

    if(text == ATBM_NULL)
    {
        return 0;
    }
    for(length = 0; length < limit; length++)
    {
        if(text[length] == '\0')
        {
            break;
        }
    }
    return length;
}
```

Measure each input with its maximum plus two, so a return value above the maximum
is rejected. Use one allocation that is reused after clearing:

```c
if((device_name == ATBM_NULL) || (ssid == ATBM_NULL) ||
   (pwd == ATBM_NULL) ||
   (device_name_len == 0) ||
   (device_name_len > ATBM_P2P_DEVICE_NAME_MAX_LEN) ||
   (ssid_len == 0) || (ssid_len > ATBM_P2P_SSID_MAX_LEN) ||
   (pwd_len < 8) || (pwd_len > ATBM_P2P_PASSWORD_MAX_LEN) ||
   !(((channel >= 1) && (channel <= 13)) ||
     ((channel >= 36) && (channel <= 165))))
{
    return -1;
}

if(g_user_p2p_config == ATBM_NULL)
{
    g_user_p2p_config = atbm_kzalloc(
        sizeof(*g_user_p2p_config), GFP_ATOMIC);
    if(g_user_p2p_config == ATBM_NULL)
    {
        return -1;
    }
}
else
{
    atbm_memset(g_user_p2p_config, 0,
        sizeof(*g_user_p2p_config));
}
```

Copy the validated lengths; the `+1` storage and zeroed allocation guarantee
terminators without copying past input strings. Validate channel fits the driver's
supported 2.4/5 GHz range before assigning it.

- [ ] **Step 3: Release user configuration after P2P deinitialization**

Make `atbm_wifi_p2p_stop()` preserve the lower-layer return, then free only after
`atbm_p2p_deinit()` has stopped P2P users of the strings:

```c
ret = atbm_p2p_deinit(priv);
if(g_user_p2p_config != ATBM_NULL)
{
    atbm_kfree(g_user_p2p_config);
    g_user_p2p_config = ATBM_NULL;
}
return ret;
```

If no VIF exists, still clear an allocated configuration before returning failure.

- [ ] **Step 4: Repair associated-client list capacity handling**

Replace the current implementation with checks that preserve the caller capacity:

```c
atbm_uint32 capacity;

if((if_id >= 2) || (pchBuf == ATBM_NULL) ||
   (uiBufSize < sizeof(WLAN_MACLIST)))
{
    return -1;
}

priv = _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv, if_id);
if(priv == ATBM_NULL)
{
    return -1;
}

capacity = maclist->count;
if(capacity > ATBMWIFI__MAX_STA_IN_AP_MODE)
{
    capacity = ATBMWIFI__MAX_STA_IN_AP_MODE;
}
atbm_memset(pchBuf, 0, uiBufSize);
maclist->count = capacity;
maclist->count = atbmwifi_get_hard_linked_macs(
    priv, maclist->ea[0].mac, maclist->count);
return 0;
```

- [ ] **Step 5: Add VIF-aware client RSSI and disconnect helpers**

Both helpers validate `if_id`, pointers, VIF existence, and AP/P2P GO mode. RSSI
searches only `ATBMWIFI__LINK_HARD` entries and writes through the output pointer.
Disconnect copies the const public MAC into a six-byte local buffer and calls:

```c
return atbmwifi_ap_deauth(priv, local_mac, ATBM_TRUE);
```

Return `-1` if the MAC is not a currently hard-linked client.

- [ ] **Step 6: Run the P2P/private-driver contract test**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1
```

Expected: private P2P and client-management assertions PASS; public implementation
assertions remain failing until Task 4.

### Task 4: Implement the public queries and P2P rollback

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- Test: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

- [ ] **Step 1: Add a current-mode-to-interface helper**

Add a locked helper used by IP and client APIs:

```c
static int atbm_wifi_active_if_id_locked(unsigned int *if_id)
{
    if(if_id == NULL)
    {
        return ATBM_WIFI_ERR_INVALID_ARG;
    }

    switch(g_wifi.status.mode)
    {
    case ATBM_WIFI_MODE_STA:
    case ATBM_WIFI_MODE_P2P_GO:
        *if_id = ATBM_WIFI_STA_IF_ID;
        return ATBM_WIFI_OK;
    case ATBM_WIFI_MODE_AP:
        *if_id = ATBM_WIFI_AP_IF_ID;
        return ATBM_WIFI_OK;
    default:
        return ATBM_WIFI_ERR_NOT_READY;
    }
}
```

- [ ] **Step 2: Implement STA association and connection information queries**

`atbm_wifi_is_connected()` validates the output pointer and STA mode, then maps
`atbm_wifi_isconnected(0)` to `0` or `1`.

`atbm_wifi_get_connection_info()` checks STA mode and association, zeroes both
public and private result structures, calls
`atbm_wifi_get_connected_info_vif(0, &raw)`, bounds `raw.Ssid_len` to 32, and
copies SSID, BSSID, channel, RSSI, and PHY rate. `atbm_wifi_get_rssi()` calls the
connection-info function and copies `info.rssi` so both APIs have identical
readiness semantics.

- [ ] **Step 3: Implement LwIP-independent IP output**

Resolve the active netif with `atbm_wifi_netif_get_vif(if_id)`. Copy the three
IPv4 addresses using LwIP byte macros rather than exposing `ip_addr_t`:

```c
info->ip[0] = ip4_addr1(&netif->ip_addr);
info->ip[1] = ip4_addr2(&netif->ip_addr);
info->ip[2] = ip4_addr3(&netif->ip_addr);
info->ip[3] = ip4_addr4(&netif->ip_addr);
```

Repeat for `netif->netmask` and `netif->gw`. Return
`ATBM_WIFI_ERR_NOT_READY` when there is no active interface.

- [ ] **Step 4: Implement AP/P2P GO client list and disconnect**

`atbm_wifi_get_clients()` requires non-null output/count and non-zero capacity.
Accept only AP/P2P GO, choose VIF 1/0, initialize a stack `WLAN_MACLIST` with:

```c
memset(&raw, 0, sizeof(raw));
raw.count = (max_clients < ATBM_WIFI_MAX_STA_IN_AP_MODE)
    ? max_clients : ATBM_WIFI_MAX_STA_IN_AP_MODE;
```

Call `atbm_wifi_get_associated_client_list_vif()`, copy each MAC, and query each
RSSI through `atbm_wifi_get_client_rssi_vif()`. If RSSI lookup fails after a
concurrent departure, store the driver's existing unknown sentinel `100` and keep
the MAC snapshot. Return the copied count.

`atbm_wifi_disconnect_client()` chooses the same VIF and maps the private helper's
result to `ATBM_WIFI_OK` or `ATBM_WIFI_ERR_NOT_READY`.

- [ ] **Step 5: Make P2P start failure rollback stage-aware**

Track whether P2P task and GO start completed:

```c
int p2p_started = 0;
int go_started = 0;
```

On failure, stop only completed stages in reverse order, always close interface 0,
and keep the original public error:

```c
fail:
    atbm_wifi_network_server_stop_locked();
    if(go_started)
    {
        atbm_wifi_p2p_go_stop();
    }
    if(p2p_started)
    {
        atbm_wifi_p2p_stop();
    }
    atbm_wifi_off(ATBM_WIFI_STA_IF_ID);
    g_wifi.status.state = ATBM_WIFI_STATE_ERROR;
```

Set `p2p_started` immediately after successful `atbm_wifi_p2p_start()` and
`go_started` immediately after successful `atbm_wifi_p2p_go_start()`.

- [ ] **Step 6: Keep hot switching firmware-resident**

Do not call `atbm_wifi_hw_deinit()` from mode changes. Confirm
`atbm_wifi_stop_current_locked()` uses DHCP stop → GO stop → P2P stop → interface
off for P2P, and `atbm_wifi_prepare_start_locked()` calls only
`atbm_wifi_on(ATBM_WIFI_STA_MODE)` when P2P switches to STA.

- [ ] **Step 7: Run all three interface/P2P tests**

Run the three Task 1 commands.

Expected: all PASS.

### Task 5: Add a P2P application example without changing default boot mode

**Files:**
- Modify: `Project/wifi_demo/atbm_wifi_probe_demo.c`
- Test: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

- [ ] **Step 1: Add example-only P2P constants**

Add beside the existing AP defaults:

```c
#define ATBM_P2P_DEVICE_NAME    "GP-ATBM6165"
#define ATBM_P2P_MODE_SSID      "DIRECT-GP-ATBM"
#define ATBM_P2P_MODE_PASSWORD  "12345678"
#define ATBM_P2P_MODE_CHANNEL   36
```

- [ ] **Step 2: Add a public-API-only P2P entry function**

Add:

```c
INT32S atbm_wifi_p2p_mode_enable(void)
{
    static const atbm_wifi_p2p_config_t config = {
        ATBM_P2P_DEVICE_NAME,
        ATBM_P2P_MODE_SSID,
        ATBM_P2P_MODE_PASSWORD,
        ATBM_P2P_MODE_CHANNEL
    };
    int ret = atbm_wifi_start_p2p(&config);

    DBG_PRINT("ATBM WiFi P2P GO start ret=%d ssid=%s channel=%d\r\n",
        ret, config.ssid, config.channel);
    return (ret == ATBM_WIFI_OK) ? STATUS_OK : STATUS_FAIL;
}
```

Do not print the password. Do not change the current call to
`atbm_wifi_ap_mode_enable()` in application startup; the new function is an
application-selectable example, not a product-default behavior change.

- [ ] **Step 3: Extend the focused test to protect the example boundary**

Assert the demo contains `atbm_wifi_start_p2p`, contains no private WiFi header,
and contains no password-format log such as `passwd=%s` or `password=%s`.

- [ ] **Step 4: Run the focused test**

Run the Task 3 command.

Expected: PASS.

### Task 6: Build the WiFi library and verify exported symbols

**Files:**
- Generated: `output/wireless_libs/internal/libatbm_wifi.a`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Verify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`

- [ ] **Step 1: Run the relevant contract tests**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Expected: all PASS and the existing WiFi source membership remains unchanged
because both modified `.c` files are already in the manifest/project.

- [ ] **Step 2: Build the wireless archives**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected: exit code 0 and `Wireless library build completed.`. This rebuilds the
existing BTstack archive as part of the established script but does not link it
into the main firmware.

- [ ] **Step 3: Verify all new public symbols in the customer WiFi archive**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe' `
  -g --defined-only `
  'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a' |
  Select-String 'atbm_wifi_(is_connected|get_connection_info|get_rssi|get_ip_info|get_clients|disconnect_client)$'
```

Expected: six defined symbols.

- [ ] **Step 4: Verify the archive contains no plaintext demo password log format**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-strings.exe' `
  'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a' |
  Select-String 'passwd=%s|password=%s'
```

Expected: no new application-wrapper password logging. If an unrelated legacy
driver string is found, record its exact object/source instead of broadening this
task beyond the touched API path.

- [ ] **Step 5: Hand off main-firmware and board verification**

Ask the user to compile `Project/GPCV2178_Turnkey_SDK` in G+IDE so the refreshed
`libatbm_wifi.a` is linked into `project.bin`. Request these board checks:

```text
1. cold P2P GO: phone discovers, connects, gets IP, and passes traffic
2. P2P client connect/disconnect events and client-list/RSSI query
3. disconnect_client(mac) produces deauth and removes the client
4. STA -> P2P GO -> STA switch without firmware reload
5. ten repeated P2P start/stop cycles without memory loss or duplicate DHCP server
6. AP client list and disconnect still work
```

Do not claim board behavior is verified until the user supplies the G+IDE build
result and serial log.

### Execution notes

This workspace is not a Git repository, so commit steps are intentionally omitted.
Use `apply_patch` for every source/test/document edit. Preserve all unrelated user
changes. The main firmware build remains a user-run G+IDE step as previously agreed.
