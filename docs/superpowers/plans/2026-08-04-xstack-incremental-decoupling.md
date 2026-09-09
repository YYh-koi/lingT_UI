# xStack Incremental Decoupling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Decouple structured Bluetooth events and AT command metadata while preserving every public API and AT byte sequence, and fix the demo startup order to enable Bluetooth before setting its name.

**Architecture:** Add a fixed-capacity synchronous internal Event Router with Manager and AT sinks. Move command identity/forms/domain metadata into static tables while retaining the current parameter validators and domain handlers. Keep PCM on its existing direct callback path and make the demo wait for `+PWRSTAT=1` before submitting `AT+NAME=GP-BT-ATBM6165`.

**Tech Stack:** C99/C11, BTstack Classic profiles, FreeRTOS queues/tasks, GPCV2178 port operations, PowerShell contract tests, Generalplus ARM GCC 4.9.3 and generated makefiles.

---

## File map

- Create `btstack/events/bt_xstack_event_router.h`: generic internal event type and router API.
- Create `btstack/events/bt_xstack_event_router.c`: static sink registry and synchronous fanout.
- Create `btstack/events/tests/test_bt_xstack_event_router_contract.ps1`: router behavior/build contract.
- Modify `btstack/adapter/btstack_adapter.c`: initialize/reset the Router with the adapter lifecycle.
- Modify `btstack/adapter/bt_xstack_manager.c`: register a Manager sink and derive power state from routed events.
- Modify `btstack/adapter/bt_xstack_manager_internal.h`: remove the backend-owned power notification API.
- Modify `btstack/at/btstack_at_internal.h`: use generic event aliases during migration.
- Modify `btstack/at/btstack_at.c`: register the AT sink and copy routed events into the existing AT queue.
- Modify `btstack/at/btstack_at_backend.c`: publish normalized profile events through the Router and use table-driven domain dispatch.
- Modify `btstack/at/btstack_at_protocol.c`: consume the generic event type and table-drive simple event names.
- Create `btstack/at/btstack_at_command_table.h/.c`: command name, ID, allowed forms, and validation metadata.
- Create `btstack/at/btstack_at_dispatch_table.h/.c`: command ID to General/HFP/A2DP-AVRCP domain mapping.
- Modify `btstack/at/btstack_at_parser.c`: replace duplicate name/form switches with descriptor lookup while retaining concrete validators.
- Modify `btstack/at/tests/test_btstack_at_parser.c`: descriptor-backed positive and negative regression cases.
- Modify `btstack/at/tests/test_btstack_at_protocol.c`: exact byte compatibility for generic events.
- Modify `btstack/at/tests/test_btstack_at_backend_contract.ps1`: require Router publishing and table dispatch; reject direct Manager/AT event calls.
- Create `btstack/at/tests/test_btstack_at_tables_contract.ps1`: require complete, unique command metadata and domain coverage.
- Modify `Project/wifi_demo/bt_xstack_demo.c`: explicit startup state machine.
- Modify `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`: require BTEN-before-NAME event-driven order.
- Modify `btstack/adapter/Makefile.inc`, `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`, and `output/wireless_libs/build/Xstack_Library_Debug.mk`: compile all new units.
- Modify `btstack/adapter/tests/test_bt_xstack_build_integration.ps1`: require each new unit exactly once.

The workspace has no Git metadata, so every task ends with a focused test/build checkpoint instead of a commit. Do not remove or overwrite unrelated user changes.

### Task 1: Add the internal Event Router

**Files:**
- Create: `btstack/events/bt_xstack_event_router.h`
- Create: `btstack/events/bt_xstack_event_router.c`
- Create: `btstack/events/tests/test_bt_xstack_event_router_contract.ps1`

- [ ] **Step 1: Write the failing Router contract**

Create a PowerShell contract that requires the exact generic type, four-slot static registry, duplicate-registration behavior, port critical-section usage, snapshot-before-callback behavior, and no heap allocation:

```powershell
$header = Get-Content -Raw 'btstack/events/bt_xstack_event_router.h'
$source = Get-Content -Raw 'btstack/events/bt_xstack_event_router.c'
foreach ($required in @('bt_xstack_event_t','bt_xstack_event_router_init',
    'bt_xstack_event_router_register','bt_xstack_event_router_publish')) {
    if (($header + $source) -notmatch $required) { throw "missing $required" }
}
if ($source -notmatch 'BT_XSTACK_EVENT_SINK_MAX\s+4u') { throw 'router capacity is not four' }
if ($source -notmatch 'btstack_port_ops_get_ops') { throw 'router does not use port critical callbacks' }
if ($source -match '\b(?:malloc|calloc|realloc|free)\s*\(') { throw 'router must not use heap memory' }
if ($source -notmatch 'snapshot') { throw 'callbacks are not invoked from a registry snapshot' }
```

- [ ] **Step 2: Run the contract and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/events/tests/test_bt_xstack_event_router_contract.ps1
```

Expected: FAIL because the Router files do not exist.

- [ ] **Step 3: Define the generic event and Router API**

Use the exact first-stage shape below so AT code can migrate without field conversions:

```c
#define BT_XSTACK_EVENT_TEXT_MAX 64u
#define BT_XSTACK_EVENT_ADDRESS_LEN 6u

typedef enum {
    BT_XSTACK_EVENT_DEVSTAT = 0,
    BT_XSTACK_EVENT_PWRSTAT,
    BT_XSTACK_EVENT_SCAN_RESULT,
    BT_XSTACK_EVENT_SCAN_END,
    BT_XSTACK_EVENT_PAIR_REQUEST,
    BT_XSTACK_EVENT_PAIRED,
    BT_XSTACK_EVENT_MICMUTED,
    BT_XSTACK_EVENT_HFPSTAT,
    BT_XSTACK_EVENT_HFPDEV,
    BT_XSTACK_EVENT_HFPAUDIO,
    BT_XSTACK_EVENT_HFPVR,
    BT_XSTACK_EVENT_HFPSIG,
    BT_XSTACK_EVENT_HFPNET,
    BT_XSTACK_EVENT_HFPROAM,
    BT_XSTACK_EVENT_HFPBATT,
    BT_XSTACK_EVENT_HFPMANU,
    BT_XSTACK_EVENT_HFPNUM,
    BT_XSTACK_EVENT_HFPIBR,
    BT_XSTACK_EVENT_HFPRING,
    BT_XSTACK_EVENT_HFPLOST,
    BT_XSTACK_EVENT_HFPMTPX,
    BT_XSTACK_EVENT_A2DPSTAT,
    BT_XSTACK_EVENT_A2DPDEV,
    BT_XSTACK_EVENT_A2DPPCMCONFIG,
    BT_XSTACK_EVENT_AVRCPSTAT,
    BT_XSTACK_EVENT_A2DPLOST,
    BT_XSTACK_EVENT_PLAYSTAT,
    BT_XSTACK_EVENT_PLAYMODE,
    BT_XSTACK_EVENT_TRACKSTAT,
    BT_XSTACK_EVENT_TRACKINFO,
    BT_XSTACK_EVENT_ABSVOL
} bt_xstack_event_id_t;

typedef struct {
    bt_xstack_event_id_t id;
    uint32_t values[3];
    uint8_t value_count;
    int32_t signed_value;
    uint8_t address[BT_XSTACK_EVENT_ADDRESS_LEN];
    uint32_t class_of_device;
    uint8_t address_type;
    char text[BT_XSTACK_EVENT_TEXT_MAX];
    char text2[BT_XSTACK_EVENT_TEXT_MAX];
    char text3[BT_XSTACK_EVENT_TEXT_MAX];
} bt_xstack_event_t;

typedef void (*bt_xstack_event_sink_t)(const bt_xstack_event_t *, void *);
int bt_xstack_event_router_init(void);
void bt_xstack_event_router_reset(void);
int bt_xstack_event_router_register(bt_xstack_event_sink_t sink, void *context);
int bt_xstack_event_router_publish(const bt_xstack_event_t *event);
```

- [ ] **Step 4: Implement fixed-capacity best-effort fanout**

Use four static `{ sink, context }` entries. `init/reset` zeroes the table inside the port critical section. `register(NULL, ...)` returns `BTSTACK_STATUS_INVALID_ARGUMENT`; an exact duplicate returns success; a full table returns `BTSTACK_STATUS_NO_MEMORY`. `publish(NULL)` returns invalid argument. `publish` copies active entries to a local `snapshot[4]` inside the critical section, exits the section, then invokes each callback exactly once. Zero sinks is a successful no-op.

- [ ] **Step 5: Run the Router contract and compile the unit**

Run Step 2, then compile without linking:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-gcc.exe' `
  -std=c99 -Wall -Wextra -Werror -Ibtstack/include -Ibtstack `
  -c btstack/events/bt_xstack_event_router.c -o tmp/tests/bt_xstack_event_router.o
```

Expected: contract exits 0; compiler exits 0 with no diagnostics.

### Task 2: Route Profile events to Manager and AT sinks

**Files:**
- Modify: `btstack/adapter/btstack_adapter.c`
- Modify: `btstack/adapter/bt_xstack_manager.c`
- Modify: `btstack/adapter/bt_xstack_manager_internal.h`
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: `btstack/at/btstack_at.c`
- Modify: `btstack/at/btstack_at_backend.c`
- Modify: `btstack/at/btstack_at_protocol.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/adapter/tests/test_bt_xstack_manager_contract.ps1`

- [ ] **Step 1: Change contracts to require routed ownership and verify RED**

Require `btstack_at_backend.c` to contain `bt_xstack_event_router_publish` and reject both `btstack_at_post_event(` and `bt_xstack_manager_on_power_state(`. Require Manager to register a Router sink, handle only `BT_XSTACK_EVENT_PWRSTAT`, and remove `bt_xstack_manager_on_power_state` from the internal header.

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/at/tests/test_btstack_at_backend_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/adapter/tests/test_bt_xstack_manager_contract.ps1
```

Expected: both fail on the old direct calls/API.

- [ ] **Step 2: Add migration aliases without changing numeric values**

In `btstack_at_internal.h`, include the Router header and replace the AT-owned event enum/struct with compatibility aliases:

```c
typedef bt_xstack_event_id_t btstack_at_event_id_t;
typedef bt_xstack_event_t btstack_at_event_t;
#define BTSTACK_AT_EVENT_DEVSTAT BT_XSTACK_EVENT_DEVSTAT
/* Define one alias for every existing event ID through ABSVOL. */
```

Keep `btstack_at_post_event(const btstack_at_event_t *)` private for the AT sink only; Backend must no longer call it.

- [ ] **Step 3: Register the AT sink**

Add this private callback in `btstack_at.c`:

```c
static void btstack_at_event_sink(const bt_xstack_event_t *event, void *context)
{
    (void)context;
    (void)btstack_at_post_event(event);
}
```

Register it once in `btstack_at_start()` after all three static queues have been created successfully and before `xTaskCreate()` makes the AT worker available. Propagate registration failure and clear the newly created queue handles using the file's existing start-failure pattern. Exact duplicate registration remains successful, so a repeated compatible start does not consume another Router slot.

- [ ] **Step 4: Register the Manager sink and remove the direct notification**

Add a sink in `bt_xstack_manager.c` that ignores every event except PWRSTAT and applies the current state rules:

```c
static void bt_xstack_manager_event_sink(const bt_xstack_event_t *event, void *context)
{
    (void)context;
    if ((event == NULL) || (event->id != BT_XSTACK_EVENT_PWRSTAT) ||
        (event->value_count == 0u)) return;
    taskENTER_CRITICAL();
    if (event->values[0] != 0u) bt_xstack_state = BT_XSTACK_STATE_POWERED;
    else if (btstack_gpcv2178_run_loop_started())
        bt_xstack_state = BT_XSTACK_STATE_RUN_LOOP_STARTED;
    else if (bt_xstack_state != BT_XSTACK_STATE_FAILED)
        bt_xstack_state = BT_XSTACK_STATE_AT_READY;
    taskEXIT_CRITICAL();
}
```

Register it during `bt_xstack_init()` before starting the AT task. Delete `bt_xstack_manager_on_power_state()` and its declaration.

- [ ] **Step 5: Publish every normalized Backend event**

Replace every Backend call to `btstack_at_post_event(&event)` with `bt_xstack_event_router_publish(&event)`. In the power profile callback, remove the Manager call and publish a single PWRSTAT event with `value_count = 1` and `values[0] = power_on ? 1u : 0u`.

- [ ] **Step 6: Integrate Router lifecycle and verify GREEN**

Call `bt_xstack_event_router_init()` in `btstack_adapter_setup()` after `btstack_event_init()` and call `bt_xstack_event_router_reset()` during deinit. Run both Step 1 contracts plus existing protocol and AT task contracts. Expected: all exit 0 and `rg -n "bt_xstack_manager_on_power_state|btstack_at_post_event" btstack/at/btstack_at_backend.c` returns no matches.

### Task 3: Add table-driven AT command identity and allowed forms

**Files:**
- Create: `btstack/at/btstack_at_command_table.h`
- Create: `btstack/at/btstack_at_command_table.c`
- Modify: `btstack/at/btstack_at_parser.c`
- Modify: `btstack/at/tests/test_btstack_at_parser.c`
- Create: `btstack/at/tests/test_btstack_at_tables_contract.ps1`

- [ ] **Step 1: Add failing completeness and parser regression tests**

The PowerShell test parses `btstack_at_internal.h` for all command enum members from `AT` through `SABSDOWN`, then requires each ID exactly once in the descriptor table. Extend the C parser cases with invalid forms such as `AT+BTEN`, `AT+PLAY=1`, `AT+PAIR=1`, `AT+HFPANSW=1`, and valid query forms already supported.

Run the PowerShell contract and the existing ARM parser compile/link command used for `tmp/tests/btstack_at_parser*.elf`. Expected: table contract fails because files are absent; parser baseline still passes its existing compile/checkpoint.

- [ ] **Step 2: Define descriptor metadata**

Use these types:

```c
#define BTSTACK_AT_FORM_MASK_EXECUTE (1u << BTSTACK_AT_FORM_EXECUTE)
#define BTSTACK_AT_FORM_MASK_QUERY   (1u << BTSTACK_AT_FORM_QUERY)
#define BTSTACK_AT_FORM_MASK_SET     (1u << BTSTACK_AT_FORM_SET)

typedef enum {
    BTSTACK_AT_VALIDATE_NONE = 0,
    BTSTACK_AT_VALIDATE_UINT,
    BTSTACK_AT_VALIDATE_TEXT,
    BTSTACK_AT_VALIDATE_ADDRESS,
    BTSTACK_AT_VALIDATE_UINT_LIST,
    BTSTACK_AT_VALIDATE_DOMAIN_SPECIFIC
} btstack_at_validation_kind_t;

typedef struct {
    const char *name;
    uint8_t name_length;
    btstack_at_command_id_t id;
    uint8_t allowed_forms;
    btstack_at_validation_kind_t validation;
} btstack_at_command_descriptor_t;

const btstack_at_command_descriptor_t *btstack_at_command_find(
    const char *name, uint16_t name_length);
```

Populate one descriptor for every supported command. Preserve current behavior exactly: a bare command that currently becomes QUERY remains query-capable; execute-only controls remain execute-only; SET validation stays in `btstack_at_validate_set()`.

- [ ] **Step 3: Make Parser use the descriptor**

Remove `btstack_at_command_names`, `btstack_at_find_command`, `btstack_at_command_is_query_without_equals`, and `btstack_at_command_accepts_no_equals`. Parse the token, call `btstack_at_command_find()`, derive its form, then reject when:

```c
if ((descriptor->allowed_forms & (1u << command->form)) == 0u) return -1;
```

Keep all existing per-command SET validators unchanged for this phase. Unknown names remain `BTSTACK_AT_COMMAND_UNSUPPORTED` and fail with the existing case-sensitive behavior.

Use `descriptor->validation` to select the validation path: `NONE` rejects SET form, while all non-NONE kinds call the existing concrete SET validator in this phase. The descriptor therefore owns whether parameters are permitted; the existing validator continues to own command-specific limits such as volume range, address width, and text length.

- [ ] **Step 4: Verify table and parser compatibility**

Run the Step 1 table contract and parser build/test checkpoint. Run `btstack/at/tests/test_btstack_at_task_contract.ps1`. Expected: all exit 0; the new invalid-form cases are rejected and every old positive case retains the same ID/form.

### Task 4: Add table-driven Backend domain dispatch and simple event names

**Files:**
- Create: `btstack/at/btstack_at_dispatch_table.h`
- Create: `btstack/at/btstack_at_dispatch_table.c`
- Modify: `btstack/at/btstack_at_backend.c`
- Modify: `btstack/at/btstack_at_protocol.c`
- Modify: `btstack/at/tests/test_btstack_at_protocol.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/at/tests/test_btstack_at_tables_contract.ps1`

- [ ] **Step 1: Add failing unique-domain and exact-output tests**

Require every supported command ID exactly once in the dispatch table and assert these domains: General for AT through SPKVOL, HFP for HFPCFG through HFPVR, A2DP_AVRCP for A2DPSTAT through SABSDOWN. Add golden event cases for simple numeric events including PWRSTAT, HFPAUDIO, A2DPSTAT, PLAYSTAT, and ABSVOL.

Run the table, Backend, and protocol tests. Expected: table/Backend tests fail because dispatch metadata is absent; protocol baseline outputs remain the compatibility oracle.

- [ ] **Step 2: Define and populate the domain table**

```c
typedef enum {
    BTSTACK_AT_DOMAIN_GENERAL = 0,
    BTSTACK_AT_DOMAIN_HFP,
    BTSTACK_AT_DOMAIN_A2DP_AVRCP
} btstack_at_command_domain_t;

int btstack_at_command_get_domain(btstack_at_command_id_t id,
                                  btstack_at_command_domain_t *domain);
```

Return `BTSTACK_AT_STATUS_INVALID_ARGUMENT` for NULL output or unsupported/out-of-range IDs.

- [ ] **Step 3: Replace chained Backend probing**

In `btstack_at_backend_execute()`, replace the `status == 1` chain with one domain switch:

```c
switch (domain) {
case BTSTACK_AT_DOMAIN_GENERAL:
    status = btstack_at_backend_execute_general(command, &result);
    break;
case BTSTACK_AT_DOMAIN_HFP:
    status = btstack_at_backend_execute_hfp(command, &result);
    break;
case BTSTACK_AT_DOMAIN_A2DP_AVRCP:
    status = btstack_at_backend_execute_a2dp_avrcp(command, &result);
    break;
default:
    status = -1;
    break;
}
```

Retain each domain's existing internal switch and response semantics.

- [ ] **Step 4: Table-drive only simple event names**

Create a private `{ event_id, name }` table in `btstack_at_protocol.c` for events formatted only as a prefix plus numeric fields. Keep SCAN_RESULT, pair records, device addresses, HFP text variants, TRACKINFO and all other custom formatting in their existing explicit cases. Route a simple match through `btstack_at_writer_uint_fields()` and preserve exact CRLF bytes.

- [ ] **Step 5: Verify GREEN and byte compatibility**

Run all tests from Step 1. Expected: every command maps to one domain; Backend no longer contains the `status == 1` dispatch chain; all protocol golden bytes match exactly.

### Task 5: Make Demo startup event-driven

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Replace the old-order assertion and verify RED**

Change the demo contract to require an internal startup enum with `IDLE`, `WAIT_POWER`, `SETTING_NAME`, `STARTED`; require initial submission of only BTEN; require exact `+PWRSTAT=1` matching before NAME; reject any source ordering where NAME is submitted from `bt_xstack_demo_start()` before BTEN.

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: FAIL because current code submits NAME first.

- [ ] **Step 2: Add the startup state and exact power-line recognition**

Use:

```c
typedef enum {
    BT_XSTACK_DEMO_IDLE = 0,
    BT_XSTACK_DEMO_WAIT_POWER,
    BT_XSTACK_DEMO_SETTING_NAME,
    BT_XSTACK_DEMO_STARTED
} bt_xstack_demo_start_state_t;
```

Reuse `bt_xstack_demo_copy_event_line()`. Compare the resulting span exactly against `+PWRSTAT=1` or `+PWRSTAT=0`; do not use substring matching and do not assume NUL termination from the stack.

- [ ] **Step 3: Submit BTEN first and NAME once after power-on**

After init, key registration, audio task start, and PCM callback registration, submit only `AT+BTEN=1`. On success set `WAIT_POWER`; on failure remain `IDLE` and return the error. In the event callback, only `WAIT_POWER + PWRSTAT=1` may transition to `SETTING_NAME` and submit NAME. Successful NAME queueing transitions to `STARTED`; failed queueing returns to `WAIT_POWER`. Duplicate PWRSTAT=1 in SETTING_NAME/STARTED sends nothing. PWRSTAT=0 clears HFP state and moves any non-IDLE startup state to WAIT_POWER.

Protect each startup-state read/write with the existing FreeRTOS critical-section macros, but exit the critical section before calling `bt_xstack_send_at()`. A repeated `bt_xstack_demo_start()` returns success while the state is WAIT_POWER, SETTING_NAME, or STARTED; it does not submit a second BTEN.

- [ ] **Step 4: Verify demo and public behavior**

Run the Step 1 contract and `btstack/adapter/tests/test_btstack_default_name.ps1`. Expected: both exit 0; name remains `GP-BT-ATBM6165`, and the PCM/audio/key paths are unchanged.

### Task 6: Integrate builds and run the full regression

**Files:**
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `output/wireless_libs/build/Xstack_Library_Debug.mk`
- Modify: `btstack/adapter/tests/test_bt_xstack_build_integration.ps1`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`

- [ ] **Step 1: Add failing build-unit assertions**

Require each of these once in the Makefile source lists, G+ project XML, generated compile rules, object list, and archive prerequisites:

```text
btstack/events/bt_xstack_event_router.c
btstack/at/btstack_at_command_table.c
btstack/at/btstack_at_dispatch_table.c
```

Run `btstack/adapter/tests/test_bt_xstack_build_integration.ps1`. Expected: FAIL because the new units are not yet integrated.

- [ ] **Step 2: Add the three source units to every active build description**

Insert each source once next to its owning subsystem. Do not change the output archive path, public include paths, or main-project library selection.

- [ ] **Step 3: Run focused and subsystem regressions**

Run:

```powershell
$tests = @(
 'btstack/events/tests/test_bt_xstack_event_router_contract.ps1',
 'btstack/at/tests/test_btstack_at_tables_contract.ps1',
 'btstack/at/tests/test_btstack_at_backend_contract.ps1',
 'btstack/at/tests/test_btstack_at_task_contract.ps1',
 'btstack/adapter/tests/test_bt_xstack_manager_contract.ps1',
 'btstack/adapter/tests/test_bt_xstack_build_integration.ps1',
 'Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1',
 'btstack/ports/gpcv2178/tests/test_btstack_gpcv2178_lifecycle.ps1',
 'btstack/ports/gpcv2178/tests/test_btstack_hci_reset_policy.ps1',
 'btstack/ports/gpcv2178/tests/test_btstack_pcm_disconnect_diagnostics.ps1',
 'btstack/profiles/tests/test_btstack_a2dp_media_contract.ps1'
)
foreach ($test in $tests) {
  powershell -NoProfile -ExecutionPolicy Bypass -File $test
  if ($LASTEXITCODE -ne 0) { throw "failed: $test" }
}
```

Expected: every script exits 0. Compile the parser and protocol C tests with the same ARM commands used for the current `tmp/tests/btstack_at_parser*.elf` and `btstack_at_protocol*.elf`; both must compile/link cleanly.

- [ ] **Step 4: Rebuild `libxstack.a` from a clean verified object directory**

Resolve the exact object directory and archive under the workspace, remove only generated artifacts, then run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' `
  -f output/wireless_libs/build/Xstack_Library_Debug.mk
```

Expected: exit 0; the archive contains exactly one object for each new unit and retains A2DP/SBC objects.

- [ ] **Step 5: Relink the main firmware and inspect diagnostics**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' `
  -f output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk
```

Expected: exit 0 and a refreshed `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`. Use `arm-none-eabi-strings` to require `btstack disconnect handle=0x%04X reason=0x%02X` and reject `HCI RX EVT len=` and `HCI %s ACL len=`.

- [ ] **Step 6: Final compatibility audit**

Byte-compare `btstack/include/adapter/bt_xstack.h` with `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`; require no differences. Confirm `AT+BTEN=1` remains the only Run Loop/power-on gate, PCM still calls `bt_xstack_pcm_bridge` directly, and no public symbol contains `atbm` or `gpcv2178`.
