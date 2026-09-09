# BT xStack AT-Controlled Start Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add generic `bt_xstack_init()` and `bt_xstack_send_at()` APIs while delaying the BTstack run loop and H5/UART0 HCI power-on until `AT+BTEN=1`.

**Architecture:** `bt_xstack_manager.c` owns public initialization and runtime state. The AT task parses commands while HCI is off and calls an internal command gate before backend dispatch; the gate starts the platform run-loop task on the first `BTEN=1`, handles an already-off `BTEN=0`, and rejects other backend commands until runtime is available. The existing AT backend remains the sole executor of HCI power commands on the BTstack run-loop context.

**Tech Stack:** C99, BTstack, FreeRTOS static queues/tasks, Generalplus GPCV2178 H5/UART0 port, PowerShell contract tests, Generalplus GCC 4.9.3.

---

### Task 1: Define the generic public API and manager contract

**Files:**
- Create: `btstack/include/adapter/bt_xstack.h`
- Create: `btstack/adapter/tests/test_bt_xstack_manager_contract.ps1`

- [ ] **Step 1: Write the failing public API contract test**

Create a PowerShell test that loads `bt_xstack.h`, `bt_xstack_manager.c`, and the manager internal header, then asserts:

```powershell
$header | Should-Match 'typedef\s+void\s*\(\s*\*bt_event_callback_t\s*\)'
$header | Should-Match 'int\s+bt_xstack_init\s*\('
$header | Should-Match 'int\s+bt_xstack_send_at\s*\('
$header | Should-Not-Match 'atbm|gpcv2178'
$manager | Should-Match 'BT_XSTACK_STATE_UNINITIALIZED'
$manager | Should-Match 'BT_XSTACK_STATE_AT_READY'
$manager | Should-Match 'BT_XSTACK_STATE_RUN_LOOP_STARTED'
$manager | Should-Match 'BT_XSTACK_STATE_POWERED'
```

Implement `Should-Match` and `Should-Not-Match` as local helper functions that throw on failure so the script has no Pester dependency. Also assert that `btstack/adapter/bt_xstack.c` does not exist.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/adapter/tests/test_bt_xstack_manager_contract.ps1
```

Expected: FAIL because `bt_xstack.h` and `bt_xstack_manager.c` do not exist.

- [ ] **Step 3: Add the public header**

Declare:

```c
typedef void (*bt_event_callback_t)(
    const char *data,
    uint16_t length,
    void *context);

typedef enum {
    BT_XSTACK_STATUS_OK = 0,
    BT_XSTACK_STATUS_INVALID_ARGUMENT = -1,
    BT_XSTACK_STATUS_NOT_INITIALIZED = -2,
    BT_XSTACK_STATUS_CONFIGURATION_CONFLICT = -3,
    BT_XSTACK_STATUS_COMMAND_TOO_LONG = -4,
    BT_XSTACK_STATUS_QUEUE_FULL = -5,
    BT_XSTACK_STATUS_NOT_READY = -6,
    BT_XSTACK_STATUS_INTERNAL_ERROR = -7
} bt_xstack_status_t;

int bt_xstack_init(bt_event_callback_t event_callback, void *context);
int bt_xstack_send_at(const char *command, uint16_t length);
```

Use C++ guards and include only `<stdint.h>`.

- [ ] **Step 4: Re-run the contract test**

Expected: it still fails only because the manager implementation has not been added; the public-header checks pass.

### Task 2: Add a pre-backend AT command gate

**Files:**
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: `btstack/at/btstack_at.c`
- Modify: `btstack/at/tests/test_btstack_at_task_contract.ps1`

- [ ] **Step 1: Extend the failing AT task contract**

Add checks requiring these internal symbols and behaviors:

```powershell
@{ Name = 'command gate registration'; Pass = $source -match 'btstack_at_command_gate_register' }
@{ Name = 'command gate before backend'; Pass = $source -match 'btstack_at_command_gate\s*\(\s*&command' }
@{ Name = 'handled command response'; Pass = $source -match 'BTSTACK_AT_GATE_HANDLED' }
@{ Name = 'rejected command response'; Pass = $source -match 'BTSTACK_AT_GATE_REJECT' }
```

Run the script and expect failure on the four new checks.

- [ ] **Step 2: Define the internal gate contract**

In `btstack_at_internal.h`, after `btstack_at_command_t`, define:

```c
typedef enum {
    BTSTACK_AT_GATE_CONTINUE = 0,
    BTSTACK_AT_GATE_HANDLED = 1,
    BTSTACK_AT_GATE_REJECT = -1
} btstack_at_gate_result_t;

typedef int (*btstack_at_command_gate_t)(
    const btstack_at_command_t *command,
    void *context);

int btstack_at_command_gate_register(
    btstack_at_command_gate_t gate,
    void *context);
```

- [ ] **Step 3: Implement gate registration and invocation**

Store one gate/context pair in `btstack_at.c`. After parsing and the direct basic `AT` response, but before checking/dispatching the backend, call the gate:

```c
gate_status = btstack_at_command_gate(&command, btstack_at_command_gate_context);
if (gate_status == BTSTACK_AT_GATE_HANDLED) {
    btstack_at_emit_simple(1);
    return;
}
if (gate_status == BTSTACK_AT_GATE_REJECT) {
    btstack_at_emit_simple(0);
    return;
}
```

Reject a NULL gate during registration and allow an identical gate/context registration to be repeated.

- [ ] **Step 4: Run AT parser, protocol, task, and backend tests**

Run every PowerShell contract under `btstack/at/tests`, then compile and run the existing parser/protocol C tests using their existing harness. Expected: all existing tests pass and the new gate checks pass.

### Task 3: Split GPCV2178 static initialization from run-loop execution

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.h`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Create: `btstack/ports/gpcv2178/tests/test_btstack_gpcv2178_lifecycle.ps1`

- [ ] **Step 1: Write a failing lifecycle contract test**

Assert that the port exports exactly these internal lifecycle functions:

```c
int btstack_gpcv2178_init(void);
int btstack_gpcv2178_start_run_loop(void);
int btstack_gpcv2178_run_loop_started(void);
```

The test must also assert that `btstack_gpcv2178_init()` contains HCI/L2CAP/Profile/backend setup, that the run-loop task contains `btstack_run_loop_execute()`, and that no unconditional `btstack_gap_set_power(true)` remains in the port task.

- [ ] **Step 2: Run the lifecycle test and verify it fails**

Expected: FAIL because the old `btstack_gpcv2178_start()` API and automatic power-on still exist.

- [ ] **Step 3: Refactor the port initialization**

Move the current initialization sequence out of the task into `btstack_gpcv2178_init()`:

```c
btstack_port_ops_set_ops(&btstack_gpcv2178_ops);
btstack_run_loop_init(btstack_run_loop_gpcv2178_get_instance());
btstack_memory_init();
hci_init(hci_transport_h5_instance(btstack_uart_gpcv2178_instance()),
         &hci_transport_config);
hci_set_control(&btstack_control_gpcv2178);
l2cap_init();
btstack_adapter_setup();
btstack_classic_profiles_setup(&btstack_classic_profiles_config);
btstack_at_backend_init();
```

Preserve the existing H4 and non-DMA H5 compile-time branches. Return the first nonzero setup status and make successful repeated initialization return zero.

- [ ] **Step 4: Make the BTstack task run-loop-only**

The task entry logs startup and calls only `btstack_run_loop_execute()`. Implement `btstack_gpcv2178_start_run_loop()` with `xTaskCreate`, set the started flag only after `pdPASS`, and make repeated calls return zero without a second task.

- [ ] **Step 5: Run the port lifecycle contract**

Expected: PASS, with no automatic HCI power-on in initialization or task entry.

### Task 4: Implement `bt_xstack_manager.c`

**Files:**
- Create: `btstack/adapter/bt_xstack_manager.c`
- Create: `btstack/adapter/bt_xstack_manager_internal.h`
- Modify: `btstack/at/btstack_at_backend.c`
- Modify: `btstack/adapter/tests/test_bt_xstack_manager_contract.ps1`

- [ ] **Step 1: Add failing manager behavior checks**

Require the manager to call `btstack_gpcv2178_init`, `btstack_at_init`, `btstack_at_command_gate_register`, and `btstack_at_start`; require the gate to inspect `BTSTACK_AT_COMMAND_BTEN`; require `bt_xstack_send_at` to call `btstack_at_submit`; and require the backend stack-state event path to call `bt_xstack_manager_on_power_state`.

- [ ] **Step 2: Implement manager initialization**

`bt_xstack_init()` validates the callback, rejects a different callback/context after initialization, invokes platform/core initialization, registers the command gate, configures the AT output callback, and starts the AT task. Set `AT_READY` only after all stages succeed; set `FAILED` on a partial initialization error.

Use the existing AT task defaults:

```c
btstack_at_config_t config = {
    event_callback,
    context,
    0u,
    0u
};
```

Map `btstack_at_submit()` errors to the matching public `BT_XSTACK_STATUS_*` values.

- [ ] **Step 3: Implement the lifecycle gate**

For `BTSTACK_AT_COMMAND_BTEN` with value `1`, call `btstack_gpcv2178_start_run_loop()` once and return `CONTINUE`. For value `0` before the run loop exists, return `HANDLED`. Reject non-BTEN backend commands before run-loop startup; otherwise return `CONTINUE`.

Protect manager state transitions with `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` and never hold the critical section while creating a task or calling BTstack initialization functions.

- [ ] **Step 4: Track actual HCI working state**

Declare this internal notification:

```c
void bt_xstack_manager_on_power_state(int powered);
```

Call it from the existing `BTSTACK_GAP_EVENT_STACK_STATE` handling after synchronizing GAP state. Set Manager state to `POWERED` only when `stack_working` is true; on the OFF event return to `RUN_LOOP_STARTED` without destroying either task.

- [ ] **Step 5: Run manager and existing AT/backend contracts**

Expected: all checks pass, public names contain no ATBM/GPCV2178 identifiers, and no `bt_xstack.c` file exists.

### Task 5: Integrate the manager into both build descriptions

**Files:**
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `Project/wifi_demo/atbm_wifi_probe_demo.c`
- Modify: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`

- [ ] **Step 1: Add failing build-integration assertions**

Require `btstack/adapter/bt_xstack_manager.c` exactly once in the BTstack library project and `Makefile.inc`. Parse the main project XML and require exactly one active Debug linker node whose library ends with `libbtstack.a`.

- [ ] **Step 2: Add the manager source to the BTstack library**

Append `$(BTSTACK_ROOT)/adapter/bt_xstack_manager.c` to `BTSTACK_ADAPTER_SOURCES`, and add a matching `<Unit>` to `BTstack_Library.gpcmprj` beside the other adapter sources.

- [ ] **Step 3: Restore the main-project BTstack link**

Remove the temporary XML comment around:

```xml
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libbtstack.a" />
```

Validate the XML and verify one active occurrence.

- [ ] **Step 4: Remove WiFi-coupled automatic startup**

Delete `ATBM_WIFI_BTSTACK_HOST_START`, `g_atbm_btstack_started`, the external `btstack_gpcv2178_start` declaration, `atbm_wifi_platform_btstack_start()`, and its AP-success call. Keep WiFi AP/P2P behavior unchanged.

- [ ] **Step 5: Run build-description tests**

Expected: XML parsing succeeds, both source descriptions include the manager once, the main project links the archive once, and WiFi demo contains no BTstack startup symbol.

### Task 6: Rebuild and verify the firmware artifacts

**Files:**
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libbtstack.a`
- Generated: `output/wireless_libs/build/BTstack_Library_Debug.mk`
- Generated: `output/wireless_libs/build/GPCV2178_bt_xstack_Debug.mk`

- [ ] **Step 1: Run all BTstack contract tests**

Run all PowerShell tests under `btstack/adapter/tests`, `btstack/at/tests`, `btstack/gap/tests`, `btstack/profiles/tests`, and `btstack/ports/gpcv2178/tests`. Expected: every script exits zero.

- [ ] **Step 2: Generate and build the BTstack static-library makefile**

Use `D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.exe` with the BTstack library project, Debug configuration, and the named output makefile; invoke the bundled `make.exe`. Expected: `libbtstack.a` is rebuilt successfully.

- [ ] **Step 3: Verify exported symbols**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe' `
  'gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libbtstack.a' |
  Select-String ' bt_xstack_(init|send_at)$'
```

Expected: one global definition for each public API.

- [ ] **Step 4: Generate and build the main Debug firmware**

Generate `GPCV2178_bt_xstack_Debug.mk` from the main `.gpcmprj` and run bundled make. Expected: compile and link succeed with no unresolved BTstack symbols.

- [ ] **Step 5: Record board-only checks**

Document that hardware verification still needs these observations: `bt_xstack_init()` does not open UART0; first `AT+BTEN=1` starts one run-loop task and produces `OK` plus `+PWRSTAT:1`; `BTEN=0` closes UART0 without deleting the task; a later `BTEN=1` reuses the task and restores HCI.

This workspace has no `.git` metadata, so the normal per-task commit steps cannot be performed. Preserve verification output and the exact modified-file list as the implementation checkpoint.
