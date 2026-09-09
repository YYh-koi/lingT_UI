# ATBM WiFi Interface Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a customer-facing `atbm_wifi_interface.h` API that starts and stops ATBM6165 AP, STA, and P2P GO modes while hiding raw driver, SDIO, LwIP, DHCP, scan-buffer, and event details inside `libatbm_wifi.a`.

**Architecture:** A self-contained public header defines stable configuration, result, status, event, and error types. A new private implementation unit in `hal` owns mode state, validates inputs, initializes the Generalplus/ATBM platform once, translates public calls to existing ATBM APIs, configures LwIP/DHCP, and converts raw driver events. The existing demo becomes a thin default-configuration caller, while both G+ projects and the scripted manifest compile the same implementation source.

**Tech Stack:** C99, G+ IDE ARM GCC, FreeRTOS/CMSIS-RTOS, ATBM6165 driver, LwIP, PowerShell contract tests and packaging scripts.

---

### Task 1: Add failing public API contract tests

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_public_api.ps1`
- Create: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`

- [ ] **Step 1: Extend the standalone-header test**

Add `atbm_wifi_interface.h` to the header compile list:

```powershell
foreach ($header in @(
    'atbm_wifi_types.h',
    'atbm_wifi_driver_api.h',
    'atbm_api.h',
    'atbm_wifi_interface.h'
)) {
```

- [ ] **Step 2: Add an exact contract test**

Create a PowerShell test that loads `atbm_wifi_interface.h` and asserts all eleven public symbols, the three configuration types, scan/status/event types, callback type, and the fixed error-code values. It must also reject includes of `atbm_hal.h`, `atbm_wifi_driver_api.h`, `atbm_api.h`, LwIP, SDIO, or FreeRTOS headers.

Core assertions:

```powershell
$requiredFunctions = @(
    'atbm_wifi_start_ap',
    'atbm_wifi_stop_ap',
    'atbm_wifi_start_sta',
    'atbm_wifi_stop_sta',
    'atbm_wifi_start_p2p',
    'atbm_wifi_stop_p2p',
    'atbm_wifi_scan',
    'atbm_wifi_get_status',
    'atbm_wifi_get_mac',
    'atbm_wifi_register_event_callback',
    'atbm_wifi_deinit'
)
foreach ($name in $requiredFunctions) {
    Assert-Match -Text $headerText -Pattern ("\b{0}\s*\(" -f $name) -Message "$name is declared"
}
Assert-NotMatch -Text $headerText `
    -Pattern '#include\s+"(atbm_hal|atbm_wifi_driver_api|atbm_api|lwip|.*sdio|FreeRTOS)' `
    -Message 'public header has no private includes'
```

- [ ] **Step 3: Run tests and verify the new contract fails**

Run:

```powershell
pwsh -NoProfile -File tools/wireless_lib/tests/test_atbm_public_api.ps1
pwsh -NoProfile -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
```

Expected: failure because `atbm_wifi_interface.h` does not exist.

### Task 2: Create the self-contained public header

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_interface.h`

- [ ] **Step 1: Define the complete public ABI**

Create the guarded C/C++ compatible header. It includes only `atbm_wifi_types.h` and defines:

```c
typedef enum {
    ATBM_WIFI_SECURITY_OPEN = 0,
    ATBM_WIFI_SECURITY_WEP,
    ATBM_WIFI_SECURITY_WPA_PSK,
    ATBM_WIFI_SECURITY_WPA2_PSK,
    ATBM_WIFI_SECURITY_WPA_WPA2_PSK
} atbm_wifi_security_t;

typedef enum {
    ATBM_WIFI_MODE_NONE = 0,
    ATBM_WIFI_MODE_STA,
    ATBM_WIFI_MODE_AP,
    ATBM_WIFI_MODE_P2P_GO
} atbm_wifi_interface_mode_t;

typedef enum {
    ATBM_WIFI_STATE_STOPPED = 0,
    ATBM_WIFI_STATE_STARTING,
    ATBM_WIFI_STATE_STARTED,
    ATBM_WIFI_STATE_CONNECTED,
    ATBM_WIFI_STATE_ERROR
} atbm_wifi_state_t;

typedef enum {
    ATBM_WIFI_OK = 0,
    ATBM_WIFI_ERR_INVALID_ARG = -1,
    ATBM_WIFI_ERR_INVALID_STATE = -2,
    ATBM_WIFI_ERR_PLATFORM_INIT = -3,
    ATBM_WIFI_ERR_DRIVER_INIT = -4,
    ATBM_WIFI_ERR_START_FAILED = -5,
    ATBM_WIFI_ERR_NETWORK_FAILED = -6,
    ATBM_WIFI_ERR_NO_MEMORY = -7,
    ATBM_WIFI_ERR_NOT_READY = -8
} atbm_wifi_error_t;
```

Add the AP, STA, P2P, scan result, status, event, callback types and these declarations:

```c
int atbm_wifi_start_ap(const atbm_wifi_ap_config_t *config);
int atbm_wifi_stop_ap(void);
int atbm_wifi_start_sta(const atbm_wifi_sta_config_t *config);
int atbm_wifi_stop_sta(void);
int atbm_wifi_start_p2p(const atbm_wifi_p2p_config_t *config);
int atbm_wifi_stop_p2p(void);
int atbm_wifi_scan(atbm_wifi_scan_result_t *results,
                   unsigned int max_results,
                   unsigned int *result_count);
int atbm_wifi_get_status(atbm_wifi_status_t *status);
int atbm_wifi_get_mac(unsigned char mac[6]);
int atbm_wifi_register_event_callback(atbm_wifi_event_callback_t callback,
                                      void *user_data);
int atbm_wifi_deinit(void);
```

- [ ] **Step 2: Run public contract tests**

Run the two Task 1 commands.

Expected: both pass; the new header compiles with only the public `api` include directory.

### Task 3: Add failing implementation/build membership tests

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`
- Modify: `tools/wireless_lib/wireless_sources.json`

- [ ] **Step 1: Update the expected source contract first**

Add:

```json
"gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c"
```

to the WiFi source manifest, keep lexical grouping with other `hal` units, and change WiFi expected count from `92` to `93`.

In `test_wireless_library_projects.ps1`, change:

```powershell
Assert-Equal -Expected 93 -Actual $wifiProject.Sources.Count -Message 'WiFi source count'
```

- [ ] **Step 2: Run the project contract test**

Run:

```powershell
pwsh -NoProfile -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Expected: failure because the source and G+ project unit do not exist yet.

### Task 4: Implement the business interface in the WiFi library

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`

- [ ] **Step 1: Add internal state and exact validation rules**

Use a single private context:

```c
typedef struct {
    atbm_mutex lock;
    ATBM_BOOL lock_ready;
    ATBM_BOOL sdio_ready;
    ATBM_BOOL lwip_ready;
    ATBM_BOOL driver_ready;
    ATBM_BOOL dhcps_ready;
    atbm_wifi_status_t status;
    atbm_wifi_event_callback_t callback;
    atbm_void *callback_user_data;
} atbm_wifi_interface_context_t;
```

Validation rules:

- SSID: 1 to 32 bytes.
- WPA/WPA2 password: 8 to 63 bytes.
- Open security: `password` may be `NULL` or empty.
- AP/P2P channel: 1 to 13 or 36 to 165.
- BSSID: optional; when supplied, exactly six binary bytes are read.
- Scan requires non-null results, non-zero capacity, and non-null count.

- [ ] **Step 2: Add platform initialization**

`atbm_wifi_platform_init()` performs the current proven sequence from
`Project/wifi_demo/atbm_wifi_probe_demo.c`:

```c
#if C_WIFI_ON_OFF_KEY
gpio_write_io(WIFI_LDO_EN_PIN, DATA_HIGH);
osDelay(50);
#endif
drvl1_sdio_config_bit_set(SDIO_WIFI_DEVICE_ID, ATBM_SDIO_BUS_BITS);
#if (ATBM_SDIO_BUS_BITS == 1)
drvl1_sdio_config_one_bit_intr_set(
    SDIO_WIFI_DEVICE_ID, EXTB, EXTB_IRQ_MUX3);
#endif
if (drvl2_sdio_init(SDIO_WIFI_DEVICE_ID) < 0) {
    return ATBM_WIFI_ERR_PLATFORM_INIT;
}
LwIP_Init();
if (atbm_wifi_hw_init() < 0 || atbm_wifi_get_init_flag() == 0) {
    return ATBM_WIFI_ERR_DRIVER_INIT;
}
```

Each completed step sets its context flag so repeated calls are idempotent and failed starts can roll back.

- [ ] **Step 3: Implement AP start/stop**

Map public security to the existing authentication/encryption pair, call
`atbm_wifi_on(ATBM_WIFI_AP_MODE)`, call `atbm_wifi_ap_create()`, configure
interface 1 using the existing `GP_AUTH_IP_ADDR*`, `NETMASK_ADDR*`, and
`GW_ADDR*` constants, then call `dhcps_init()`.

Stop in reverse:

```c
if (g_wifi.dhcps_ready) {
    dhcps_deinit();
    g_wifi.dhcps_ready = ATBM_FALSE;
}
atbm_wifi_off(1);
```

- [ ] **Step 4: Implement STA start/stop**

Map security, call `atbm_wifi_on(ATBM_WIFI_STA_MODE)`, then:

```c
ret = atbm_wifi_sta_join_ap(
    (char *)config->ssid,
    (char *)config->bssid,
    auth_mode,
    encryption,
    (char *)config->password);
```

The existing ATBM LwIP adapter starts DHCP Client on network-enable. Stop with
`atbm_wifi_sta_disjoin_ap()` followed by `atbm_wifi_off(0)` only when the
driver has not already removed the interface.

- [ ] **Step 5: Implement P2P GO start/stop**

Start in this exact order:

```c
ret = atbm_wifi_p2p_user_config(
    (char *)config->device_name,
    (char *)config->ssid,
    (char *)config->password,
    config->channel);
if (ret == 0) ret = atbm_wifi_p2p_start();
if (ret == 0) ret = atbm_wifi_p2p_go_start();
```

Then configure the actual P2P netif (the current driver assigns P2P to if_id 0)
and start DHCP Server. Stop with
`atbm_wifi_p2p_go_stop()`, `atbm_wifi_p2p_stop()`, DHCP teardown, and
`atbm_wifi_off(0)`. Add the currently implemented but undeclared
`atbm_wifi_p2p_go_stop()` prototype privately in the source rather than
exposing it to customers.

- [ ] **Step 6: Implement scan, queries, events, and deinit**

Allocate a 2732-byte raw scan buffer with `atbm_kzalloc`, call
`atbm_wifi_scan_network()`, bound `WLAN_SCAN_RESULT.count` by both buffer
capacity and caller capacity, and copy fields into public results. Free the
raw buffer on every path.

Register one raw ATBM callback and translate:

```c
ATBM_WIFI_SCANDONE_EVENT      -> ATBM_WIFI_EVENT_SCAN_DONE
ATBM_WIFI_ASSOC_EVENT         -> ATBM_WIFI_EVENT_STA_CONNECTED
ATBM_WIFI_DEAUTH_EVENT        -> ATBM_WIFI_EVENT_STA_DISCONNECTED
ATBM_WIFI_DEASSOC_EVENT       -> ATBM_WIFI_EVENT_STA_DISCONNECTED
ATBM_WIFI_ASSOCRSP_TXOK_EVENT -> ATBM_WIFI_EVENT_CLIENT_CONNECTED
```

For AP/P2P deauth, emit `ATBM_WIFI_EVENT_CLIENT_DISCONNECTED`. Copy the six-byte
MAC before invoking the customer callback. Never expose `priv` or `param`.

`atbm_wifi_deinit()` stops the active mode, calls `atbm_wifi_hw_deinit()`,
clears state, powers WiFi down when configured, and emits STOPPED.

- [ ] **Step 7: Syntax-compile the new implementation**

Use the main Debug compiler configuration through the existing build script:

```powershell
pwsh -NoProfile -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected at this checkpoint: the compile plan reaches the new source; any API or include mismatch is fixed before project integration proceeds.

### Task 5: Add the source to both G+ configurations and migrate the demo

**Files:**
- Modify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Modify: `Project/wifi_demo/atbm_wifi_probe_demo.c`

- [ ] **Step 1: Add the implementation unit**

Add this unit beside `atbm_wifi_driver_api.c` in the independent project:

```xml
<Unit filename="../../../gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c">
    <Option compilerVar="CC" />
</Unit>
```

Do not add the source to the main project. Its ATBM source block is commented
out and the active configuration links `libatbm_wifi.a`; compiling the unit
again would create duplicate public symbols.

- [ ] **Step 2: Reduce the demo to a default AP caller**

Replace its direct SDIO/LwIP/driver implementation with:

```c
#include "atbm_wifi_interface.h"

INT32S atbm_wifi_ap_mode_enable(void)
{
    static const atbm_wifi_ap_config_t config = {
        "wlan_ap_gp",
        "12345678",
        36,
        ATBM_WIFI_SECURITY_WPA2_PSK,
        0
    };

    return atbm_wifi_start_ap(&config) == ATBM_WIFI_OK
        ? STATUS_OK
        : STATUS_FAIL;
}
```

Keep the existing BTstack host-start helper in the demo and call it only after
the public WiFi start succeeds; the WiFi library must not gain a BTstack link
dependency.

- [ ] **Step 3: Keep the existing application entry stable**

Keep `atbm_wifi_ap_mode_enable()` as the application demo entry so `main.c`
does not need product-configuration details. The demo implementation delegates
to `atbm_wifi_start_ap()` and preserves the existing socket/RTSP one-time start
behavior in `main.c`.

- [ ] **Step 4: Run source/project tests**

Run:

```powershell
pwsh -NoProfile -File tools/wireless_lib/tests/test_atbm_public_api.ps1
pwsh -NoProfile -File tools/wireless_lib/tests/test_atbm_wifi_interface.ps1
pwsh -NoProfile -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
pwsh -NoProfile -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: all PASS and WiFi source count is 93.

### Task 6: Rebuild libraries, package, documentation, and patches

**Files:**
- Modify: `docs/wireless_static_library_integration.md`
- Modify: `patches/atbm_wlan_lib_layout.patch`
- Modify: `patches/wireless_static_library_integration.patch` when its baseline-covered files change
- Replace generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Rebuild: `D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`

- [ ] **Step 1: Document the customer entry point**

List `atbm_wifi_interface.h` first and show AP/STA/P2P examples that include only
that header. Mark `atbm_wifi_driver_api.h` and `atbm_api.h` as internal/legacy,
not recommended for new customer code.

- [ ] **Step 2: Build and inspect the archive**

Run:

```powershell
pwsh -NoProfile -File tools/wireless_lib/build_wireless_libs.ps1
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe' `
  -g --defined-only `
  'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a'
```

Expected: all eleven public function symbols are present and the WiFi archive
contains 93 object members.

- [ ] **Step 3: Regenerate the official customer package**

Use the existing customer staging command documented in
`docs/wireless_static_library_integration.md`, staging to a new empty directory.
After verification, move the current official customer directory to a timestamped
recoverable backup and move the new stage into its official path.

- [ ] **Step 4: Update patches for every modified existing file**

Regenerate `patches/atbm_wlan_lib_layout.patch` from the preserved pre-change
baseline so it contains every existing-file modification. New files are delivered
as files and listed in the documentation. Validate with:

```powershell
git apply --check patches/atbm_wlan_lib_layout.patch
```

against the reconstructed pre-change baseline used by the existing patch workflow.

- [ ] **Step 5: Run final verification**

Run all four PowerShell tests, rebuild both archives, run the minimal
customer-only compile/link verification, confirm no wireless implementation
`.c` exists in the customer package, verify `SHA256SUMS.txt`, and update
`output/wireless_libs/verification-report.txt`.

Expected:

- 93 WiFi objects and 59 BTstack objects;
- public header standalone compile passes;
- all public symbols are present;
- zero customer wireless implementation sources;
- zero checksum failures;
- customer-only compile and link complete successfully.

### Execution notes

This workspace is not a Git repository, so commit steps are intentionally omitted.
All existing-file edits must be made with `apply_patch` and represented in the
requested patch artifacts. Generated archives and the official customer directory
must be backed up before replacement.
