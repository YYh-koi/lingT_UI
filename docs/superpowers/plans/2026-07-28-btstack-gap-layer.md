# BTstack Independent GAP Layer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce an SDK-owned Classic GAP layer that exclusively handles native BTstack HCI/GAP operations and events before exposing stable operations and structured events to the AT backend.

**Architecture:** Extend the existing `btstack/gap/` module with a central event/state owner plus focused link, scan, and security units. The AT backend consumes only `btstack_gap_*` APIs and `btstack_gap_event_t`; the only cross-task event queue remains the existing AT event queue.

**Tech Stack:** C99, BlueKitchen BTstack, FreeRTOS, PowerShell contract tests, Generalplus ARM GCC 4.9.3, GPC project XML and wireless source manifest.

---

## File Structure

- Create `btstack/include/gap/btstack_gap.h`: public GAP state/event/core API.
- Create `btstack/include/gap/btstack_gap_security.h`: public PIN/SSP/pairing API.
- Create `btstack/gap/btstack_gap.c`: native HCI registration, state ownership, packet-to-event conversion.
- Create `btstack/gap/btstack_gap_security.c`: SSP/PIN configuration and confirmation operations.
- Create `btstack/gap/tests/test_btstack_gap_contract.ps1`: API, ownership, event-mapping, and layer-boundary checks.
- Modify `btstack/include/gap/btstack_gap_link.h`: public local-device/link controls.
- Modify `btstack/include/gap/btstack_gap_scan.h`: public Classic inquiry controls.
- Modify `btstack/gap/btstack_gap_link.c`: native local-device/link wrappers.
- Modify `btstack/gap/btstack_gap_scan.c`: native inquiry wrappers and state.
- Modify `btstack/adapter/btstack_adapter.c`: initialize and reset the central GAP layer.
- Modify `btstack/at/btstack_at_backend.c`: consume only generic GAP operations/events.
- Modify `btstack/at/tests/test_btstack_at_backend_contract.ps1`: require the new GAP boundary and reject native GAP/HCI dependencies.
- Modify `btstack/profiles/tests/check_btstack_profile_layer.ps1`: extend architecture-boundary checks to GAP and Port.
- Modify `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`: remove its business HCI handler and use GAP power control.
- Modify `tools/wireless_lib/wireless_sources.json`: add new GAP sources to the archive build.
- Modify `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`: add new GAP sources.
- Modify `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`: add new GAP sources for the current integrated project representation.

### Task 1: Add the failing GAP-layer contract

**Files:**
- Create: `btstack/gap/tests/test_btstack_gap_contract.ps1`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/profiles/tests/check_btstack_profile_layer.ps1`

- [ ] **Step 1: Write the failing GAP contract**

Create a PowerShell test that requires:

```powershell
$required = @(
    'btstack/include/gap/btstack_gap.h',
    'btstack/include/gap/btstack_gap_security.h',
    'btstack/gap/btstack_gap.c',
    'btstack/gap/btstack_gap_security.c'
)

$publicApis = @(
    'btstack_gap_init',
    'btstack_gap_set_event_callback',
    'btstack_gap_set_power',
    'btstack_gap_get_local_address',
    'btstack_gap_set_local_name',
    'btstack_gap_set_class_of_device',
    'btstack_gap_set_discoverable',
    'btstack_gap_set_connectable',
    'btstack_gap_disconnect',
    'btstack_gap_scan_start',
    'btstack_gap_scan_stop',
    'btstack_gap_set_pin',
    'btstack_gap_set_ssp_mode',
    'btstack_gap_pairing_confirm'
)

$eventNames = @(
    'BTSTACK_GAP_EVENT_STACK_STATE',
    'BTSTACK_GAP_EVENT_LOCAL_ADDRESS',
    'BTSTACK_GAP_EVENT_ACL_CONNECTED',
    'BTSTACK_GAP_EVENT_ACL_DISCONNECTED',
    'BTSTACK_GAP_EVENT_INQUIRY_RESULT',
    'BTSTACK_GAP_EVENT_INQUIRY_COMPLETE',
    'BTSTACK_GAP_EVENT_PAIRING_REQUEST',
    'BTSTACK_GAP_EVENT_PAIRING_COMPLETE'
)
```

The test must also require exactly one `hci_add_event_handler` in `btstack/gap/*.c`, require `GAP_EVENT_INQUIRY_RESULT` and all pairing/ACL source events in `btstack_gap.c`, and reject `hci_add_event_handler`, `hci_event_*`, `gap_event_*`, and raw `gap_*` calls from the AT backend.

- [ ] **Step 2: Update existing boundary tests**

Change the old positive AT checks for raw inquiry/SSP/PIN APIs into positive checks for `btstack_gap_*`, and add Port rejections for:

```powershell
'hci_add_event_handler'
'hci_power_control'
'HCI_EVENT_'
'GAP_EVENT_'
```

- [ ] **Step 3: Run tests and verify RED**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_backend_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_profile_layer.ps1
```

Expected: FAIL because the central GAP files/APIs are missing and AT/Port still contain native HCI/GAP calls.

### Task 2: Define the public GAP contract and state owner

**Files:**
- Create: `btstack/include/gap/btstack_gap.h`
- Create: `btstack/gap/btstack_gap.c`
- Modify: `btstack/adapter/btstack_adapter.c`

- [ ] **Step 1: Add public types and APIs**

Define the confirmed event enum and fixed-value event payload:

```c
typedef enum {
    BTSTACK_GAP_EVENT_STACK_STATE = 0,
    BTSTACK_GAP_EVENT_LOCAL_ADDRESS,
    BTSTACK_GAP_EVENT_ACL_CONNECTED,
    BTSTACK_GAP_EVENT_ACL_DISCONNECTED,
    BTSTACK_GAP_EVENT_INQUIRY_RESULT,
    BTSTACK_GAP_EVENT_INQUIRY_COMPLETE,
    BTSTACK_GAP_EVENT_PAIRING_REQUEST,
    BTSTACK_GAP_EVENT_PAIRING_COMPLETE
} btstack_gap_event_type_t;

typedef struct {
    btstack_gap_event_type_t type;
    int status;
    btstack_bd_addr_t address;
    uint32_t class_of_device;
    uint32_t numeric_value;
    int8_t rssi;
    uint8_t stack_working;
    uint8_t name_length;
    char name[BTSTACK_LOCAL_NAME_MAX];
} btstack_gap_event_t;
```

Expose core init/reset/callback/power/query functions. `numeric_value` is required for SSP user confirmation and avoids overloading unrelated fields.

- [ ] **Step 2: Implement one-time native registration and state reset**

Use one static `btstack_packet_callback_registration_t`, an initialized flag, a callback pointer, and a single internal state structure. `btstack_gap_init()` must clear runtime state and call `hci_add_event_handler()` only when not already registered.

- [ ] **Step 3: Implement core HCI/GAP event conversion**

Handle:

```c
BTSTACK_EVENT_STATE
HCI_EVENT_CONNECTION_COMPLETE
HCI_EVENT_DISCONNECTION_COMPLETE
HCI_EVENT_PIN_CODE_REQUEST
HCI_EVENT_USER_CONFIRMATION_REQUEST
HCI_EVENT_SIMPLE_PAIRING_COMPLETE
GAP_EVENT_INQUIRY_RESULT
GAP_EVENT_INQUIRY_COMPLETE
```

Every handler must update internal state before invoking the callback. Inquiry data must be copied into bounded fields.

- [ ] **Step 4: Wire core init/reset into the adapter**

Call `btstack_gap_init()` before the focused GAP module initializers. Call `btstack_gap_reset()` during adapter deinit after operations are quiescent.

- [ ] **Step 5: Run the GAP contract**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
```

Expected: remaining failures only for link/scan/security and AT/Port migration.

### Task 3: Implement focused link, scan, and security operations

**Files:**
- Modify: `btstack/include/gap/btstack_gap_link.h`
- Modify: `btstack/include/gap/btstack_gap_scan.h`
- Create: `btstack/include/gap/btstack_gap_security.h`
- Modify: `btstack/gap/btstack_gap_link.c`
- Modify: `btstack/gap/btstack_gap_scan.c`
- Create: `btstack/gap/btstack_gap_security.c`

- [ ] **Step 1: Add link API declarations**

Expose:

```c
int btstack_gap_set_local_name(const char *name);
int btstack_gap_set_class_of_device(uint32_t class_of_device);
int btstack_gap_set_discoverable(bool enabled);
int btstack_gap_set_connectable(bool enabled);
int btstack_gap_disconnect(void);
```

Validate name length against `BTSTACK_LOCAL_NAME_MAX`; cache accepted configuration in the GAP owner; return success when disconnect is requested without an ACL.

- [ ] **Step 2: Add scan API declarations and implementation**

Expose and implement:

```c
int btstack_gap_scan_start(uint8_t duration);
int btstack_gap_scan_stop(void);
bool btstack_gap_is_scanning(void);
```

Reject invalid duration, return busy for an already-active scan, update state only after native start succeeds, and clear state on stop or inquiry-complete.

- [ ] **Step 3: Add security API declarations and implementation**

Expose:

```c
int btstack_gap_set_pin(const char *pin);
int btstack_gap_set_ssp_mode(uint8_t mode);
int btstack_gap_pairing_confirm(
    const btstack_bd_addr_t address,
    bool accept);
```

Preserve modes exactly:

```text
0 = SSP disabled
1 = No Input/No Output + auto accept
2 = Display Yes/No + auto accept
3 = Display Yes/No + explicit AT confirmation
```

PIN requests use the cached PIN. Mode 3 emits a request with address and numeric value; other modes confirm immediately.

- [ ] **Step 4: Reapply cached configuration on WORKING**

After reading the local address, reapply cached name, COD, discoverable/connectable state, and SSP configuration before emitting stack-ready events.

- [ ] **Step 5: Run the GAP contract**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
```

Expected: GAP API/event/ownership checks pass; AT and Port boundary checks remain red.

### Task 4: Migrate the AT backend to the generic GAP contract

**Files:**
- Modify: `btstack/at/btstack_at_backend.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`

- [ ] **Step 1: Replace native includes and registration**

Include SDK GAP headers and remove the backend-owned HCI callback registration. Register:

```c
status = btstack_gap_set_event_callback(
    btstack_at_backend_gap_event);
```

- [ ] **Step 2: Replace general command operations**

Map the existing commands without changing AT behavior:

```text
BTEN → btstack_gap_set_power
ADDR → btstack_gap_get_local_address
NAME → btstack_gap_set_local_name
PIN → btstack_gap_set_pin
SSP → btstack_gap_set_ssp_mode
CFM → btstack_gap_pairing_confirm
PAIR → btstack_gap_set_discoverable
PAGE → btstack_gap_set_connectable
COD → btstack_gap_set_class_of_device
SCAN → btstack_gap_scan_start/stop
DSCA → Profile disconnects followed by btstack_gap_disconnect
```

- [ ] **Step 3: Replace packet parsing with structured event mapping**

Implement `btstack_at_backend_gap_event(const btstack_gap_event_t *)`. Keep exact AT mappings for power/device status, inquiry result/end, pair request, pair completion, and ACL state.

- [ ] **Step 4: Remove duplicate GAP state**

Remove backend ACL handle, pending pairing state, scanning state, PIN, SSP, local name/address/COD state where GAP query/setter APIs now own them. Retain only fields required for strict AT query formatting or AT-specific peer bookkeeping; such values must be refreshed through GAP APIs rather than native BTstack getters.

- [ ] **Step 5: Run AT and GAP tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_backend_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_task_contract.ps1
```

Expected: PASS.

### Task 5: Clean the Port boundary

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Modify: `btstack/profiles/tests/check_btstack_profile_layer.ps1`

- [ ] **Step 1: Remove the Port HCI callback**

Delete its `btstack_packet_callback_registration_t`, packet handler, assignment, and `hci_add_event_handler()` call.

- [ ] **Step 2: Use the SDK power API**

Replace:

```c
ret = hci_power_control(HCI_POWER_ON);
```

with:

```c
ret = btstack_gap_set_power(true);
```

Keep the separate `ret` assignment followed by `DBG_PRINT`.

- [ ] **Step 3: Run boundary tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_profile_layer.ps1
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
```

Expected: PASS with no business HCI/GAP symbols in Port.

### Task 6: Update build manifests and project files

**Files:**
- Modify: `tools/wireless_lib/wireless_sources.json`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Add source files to the wireless manifest**

Add:

```text
btstack/gap/btstack_gap.c
btstack/gap/btstack_gap_security.c
```

next to existing GAP sources, preserving deterministic ordering.

- [ ] **Step 2: Add source units to both project XML files**

Add the two `<Unit>` elements with the same compiler option as the neighboring GAP files.

- [ ] **Step 3: Run wireless tooling contract tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
powershell -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_library_projects.ps1
```

Update expected Bluetooth source counts only for the two intentional additions. Expected: PASS.

### Task 7: Compile and run the regression suite

**Files:**
- Verify all files changed in Tasks 1-6.

- [ ] **Step 1: Run all focused contract tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack/gap/tests/test_btstack_gap_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_backend_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_task_contract.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_profile_layer.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_classic_setup.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_hfp_profile.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_a2dp_profile.ps1
powershell -ExecutionPolicy Bypass -File btstack/profiles/tests/check_btstack_avrcp_profile.ps1
```

Expected: all PASS.

- [ ] **Step 2: Compile the Bluetooth archive**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/wireless_lib/build_wireless_libs.ps1
```

Expected: GCC 4.9.3 rebuilds both wireless archives and compiles all new/modified Bluetooth C files with no errors.

- [ ] **Step 3: Run strict syntax checks on changed C files**

Use the project-derived compiler options and:

```text
-std=gnu99 -Wall -Wextra -Werror -fsyntax-only
```

with the target `btstack/ports/gpcv2178/btstack_config.h` preincluded. Expected: no diagnostics from GAP, AT Backend, adapter, or Port changes.

- [ ] **Step 4: Confirm final boundaries**

Run searches that prove:

```text
AT Backend: no hci_add_event_handler, hci_event_*, gap_event_*, raw gap_* calls
Port: no business HCI/GAP event handler and no hci_power_control
GAP: contains the only SDK business HCI event registration
```

- [ ] **Step 5: Record version-control limitation**

This workspace has no Git metadata and no available SVN CLI. Do not fabricate a commit. Report the changed files and verification evidence for the user to commit with their normal SVN tooling.
