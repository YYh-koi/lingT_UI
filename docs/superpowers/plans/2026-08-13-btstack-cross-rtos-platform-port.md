# BTstack Cross-RTOS Platform Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove FreeRTOS, GPCV2178, and vendor SDK dependencies from BTstack common production code while preserving the complete public ABI and current asynchronous behavior, and make GPCV2178 the first implementation of a single-descriptor RTOS port.

**Architecture:** Common code owns fixed-capacity request/event/audio rings and all protocol behavior. Exactly one `btstack_platform_t` supplies system, worker, storage, controller, run-loop, and HCI bindings. Existing `bt_xstack_*` admission and callback contexts remain unchanged; audio downlink stays callback-driven and the only additive API is a non-blocking fixed-ring HFP uplink write.

**Tech Stack:** C99, BTstack 1.6.2, static memory, PowerShell contracts, MinGW host tests, ARM GCC, FreeRTOS GPCV2178 reference port, G+ `.gpcmprj` generation.

**Approved spec:** `docs/superpowers/specs/2026-08-13-btstack-cross-rtos-platform-port-design.md`

**Workspace note:** This workspace has no `.git` directory. Each task therefore ends with a named verification checkpoint instead of a commit command. If the tree is later imported into Git, use the checkpoint name as the commit subject.

---

## File Map

New common interfaces and implementations:

- `btstack/include/port/btstack_worker_ops.h`: RTOS-neutral worker contract.
- `btstack/include/port/btstack_stack_ops.h`: run-loop/HCI/profile binding contract.
- `btstack/include/port/btstack_platform.h`: versioned single-platform descriptor.
- `btstack/core/btstack_platform.c`: descriptor validation, activation, diagnostics, and common wrappers.
- `btstack/core/btstack_ring.h`: private fixed-item ring type.
- `btstack/core/btstack_ring.c`: allocation-free ring implementation.
- `btstack/core/btstack_call_audio_tx.c`: allocation-free HFP uplink ring and SCO drain.

New GPCV2178 port files:

- `btstack/ports/gpcv2178/btstack_platform_gpcv2178.c`: descriptor and stack/controller bindings.
- `btstack/ports/gpcv2178/btstack_rtos_gpcv2178.c`: FreeRTOS worker implementation and static task storage.

Primary common files migrated:

- `btstack/api/bt_xstack_api_core.c`: replace FreeRTOS queues/tasks/critical sections.
- `btstack/core/btstack_core_backend.c`: generic stack callback posting and locking.
- `btstack/core/btstack_core_event_mapper.c`: generic locking and audio TX lifecycle hooks.
- `btstack/core/btstack_runtime.c`: generic stack initialization and lifecycle.
- `btstack/core/btstack_event_queue.c`: consume the activated platform compatibility view.
- `btstack/core/btstack_property.c`: preserve storage behavior through the descriptor.
- `btstack/profiles/btstack_hfp_hf.c`: track SCO TX handle/length and provide bounded send-ready hooks.
- `btstack/transport/hci_transport_h4.c`, `hci_transport_h5.c`, `btstack_uart_block.h`: remove platform declarations/includes.

Build and documentation files:

- `btstack/Makefile.inc`
- `btstack/transport/Makefile.inc`
- `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- `tools/wireless_lib/wireless_sources.json`
- `btstack/README.md`
- `btstack/ports/README.md`

---

### Task 1: Establish the platform-boundary RED

**Files:**

- Create: `btstack/core/tests/test_btstack_platform_boundary.ps1`
- Modify: `btstack/core/tests/test_btstack_core_source_layout.ps1`

- [ ] **Step 1: Add a production-only dependency scanner**

Implement `test_btstack_platform_boundary.ps1` so it scans `.c` and `.h`
files below `api`, `core`, `gap`, `profiles`, and `transport`, excluding every
`tests` directory. Use these two regex groups:

```powershell
$forbiddenIncludes = '(?im)^\s*#\s*include\s*[<"](?:FreeRTOS\.h|task\.h|queue\.h|semphr\.h|cmsis_os[^>"]*|[^>"]*gpcv2178[^>"]*|gplib_[^>"]*)[>"]'
$forbiddenCalls = '(?i)\b(?:taskENTER_CRITICAL|taskEXIT_CRITICAL|xQueue[A-Z]\w*|xTask[A-Z]\w*|gplib_\w*|drv_l[12]\w*)\s*\('
```

The script must print every `path:line:text` hit and exit nonzero when any hit
exists. It must also assert that `btstack/Makefile.inc` does not add
`ports/gpcv2178` to common include paths.

- [ ] **Step 2: Extend the layout contract**

Add assertions that the final tree must contain `include/port`,
`btstack_platform.c`, and the two GPCV2178 descriptor/RTOS files exactly once
in the appropriate project. Keep the current assertions about the removed
`btstack/adapter` directory.

- [ ] **Step 3: Run the boundary test and record the expected RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\core\tests\test_btstack_platform_boundary.ps1
```

Expected: FAIL listing current FreeRTOS calls in
`bt_xstack_api_core.c`, `btstack_core_backend.c`, and
`btstack_core_event_mapper.c`, GPCV2178 includes in `btstack_runtime.c` and
`btstack_core_backend.c`, and `gplib_print_string.h` in H4/H5.

- [ ] **Step 4: Checkpoint**

Record: `test: define cross-RTOS platform boundary`.

---

### Task 2: Add the versioned platform descriptor

**Files:**

- Create: `btstack/include/port/btstack_worker_ops.h`
- Create: `btstack/include/port/btstack_stack_ops.h`
- Create: `btstack/include/port/btstack_platform.h`
- Create: `btstack/core/btstack_platform.c`
- Create: `btstack/core/tests/test_btstack_platform.c`
- Create: `btstack/core/tests/test_btstack_platform.ps1`
- Modify: `btstack/Makefile.inc`

- [ ] **Step 1: Write descriptor validation tests**

The C test supplies a descriptor with stub callbacks and asserts:

```c
CHECK(btstack_platform_activate(NULL) == BTSTACK_PLATFORM_ERROR_NULL);
CHECK(btstack_platform_activate(&bad_version) ==
      BTSTACK_PLATFORM_ERROR_ABI);
CHECK(btstack_platform_activate(&bad_size) ==
      BTSTACK_PLATFORM_ERROR_SIZE);
CHECK(btstack_platform_activate(&missing_critical) ==
      BTSTACK_PLATFORM_ERROR_REQUIRED_OP);
CHECK(btstack_platform_activate(&valid) == BTSTACK_PLATFORM_OK);
CHECK(btstack_platform_get() == &valid);
CHECK(btstack_platform_activate(&other_valid) ==
      BTSTACK_PLATFORM_ERROR_ALREADY_ACTIVE);
```

Also verify the legacy `btstack_port_ops_get_ops()` view contains the activated
system/storage/controller-identity callbacks. Compile the real
`btstack_platform.c` and `btstack_port_ops.c` with MinGW C99 `-Wall -Wextra
-Werror`.

- [ ] **Step 2: Run the new host test and confirm RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\core\tests\test_btstack_platform.ps1
```

Expected: FAIL because `port/btstack_platform.h` and
`btstack_platform.c` do not exist.

- [ ] **Step 3: Define the worker contract**

Create `btstack_worker_ops.h` with these stable types:

```c
#define BTSTACK_WORKER_WAIT_FOREVER UINT32_MAX

typedef enum {
    BTSTACK_WORKER_API = 0,
    BTSTACK_WORKER_EVENT,
    BTSTACK_WORKER_STACK,
    BTSTACK_WORKER_COUNT
} btstack_worker_id_t;

typedef void (*btstack_worker_entry_t)(void *argument);

typedef struct {
    uint32_t priority;
    uint16_t stack_words;
} btstack_worker_config_t;

typedef struct {
    void *context;
    int (*validate_config)(void *context, btstack_worker_id_t id,
        const btstack_worker_config_t *config);
    int (*start)(void *context, btstack_worker_id_t id,
        btstack_worker_entry_t entry, void *argument,
        const btstack_worker_config_t *config);
    int (*notify)(void *context, btstack_worker_id_t id,
        uint32_t signals);
    uint32_t (*wait)(void *context, btstack_worker_id_t id,
        uint32_t signal_mask, uint32_t timeout_ms);
    int (*stop)(void *context, btstack_worker_id_t id);
} btstack_worker_ops_t;
```

Define signal bits without exposing RTOS primitives:

```c
#define BTSTACK_WORKER_SIGNAL_WORK              (1u << 0)
#define BTSTACK_WORKER_SIGNAL_COMMAND_SLOT      (1u << 1)
#define BTSTACK_WORKER_SIGNAL_REQUEST_SPACE     (1u << 2)
```

- [ ] **Step 4: Define stack and platform contracts**

`btstack_stack_ops.h` must expose immutable BTstack bindings and callbacks:

```c
typedef struct {
    uint32_t current_depth;
    uint32_t max_depth;
    uint32_t full_count;
} btstack_stack_queue_diagnostics_t;

typedef struct {
    void *context;
    int (*early_init)(void *context);
    void (*late_deinit)(void *context);
    const btstack_run_loop_t *(*get_run_loop)(void *context);
    const hci_transport_t *(*get_hci_transport)(void *context);
    const void *(*get_hci_transport_config)(void *context);
    const btstack_classic_profiles_config_t *
        (*get_classic_profiles_config)(void *context);
    int (*after_hci_init)(void *context);
    bool (*post_callback)(void *context,
        btstack_context_callback_registration_t *callback);
    void (*get_diagnostics)(void *context,
        btstack_stack_queue_diagnostics_t *diagnostics);
} btstack_stack_ops_t;
```

`btstack_platform.h` defines version 1 and composes existing compatible ops:

```c
#define BTSTACK_PLATFORM_ABI_VERSION 1u

typedef struct {
    btstack_port_ops_controller_ops_t identity;
    const btstack_control_t *control;
} btstack_platform_controller_ops_t;

typedef struct {
    uint16_t abi_version;
    uint16_t struct_size;
    const char *name;
    btstack_port_ops_system_ops_t system;
    btstack_worker_ops_t workers;
    btstack_port_ops_storage_ops_t storage;
    btstack_platform_controller_ops_t controller;
    btstack_stack_ops_t stack;
} btstack_platform_t;

const btstack_platform_t *btstack_platform_default(void);
```

Define internal validation status values, diagnostics, activation/getter,
critical wrappers, worker wrappers, and stack-post wrapper in the same header.

- [ ] **Step 5: Implement activation and compatibility view**

`btstack_platform_activate()` validates version, `struct_size`, name, paired
critical callbacks, time callback, all five worker callbacks, early/late stack
hooks, run-loop, transport, config, controller control, and stack-post
callbacks. After
validation it calls `btstack_port_ops_set_ops()` with a local table assembled
from descriptor system/storage/controller identity fields. Do not allocate.

Provide:

```c
void btstack_platform_enter_critical(void);
void btstack_platform_exit_critical(void);
int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals);
uint32_t btstack_platform_wait(btstack_worker_id_t id,
    uint32_t mask, uint32_t timeout_ms);
bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback);
```

- [ ] **Step 6: Run host tests**

Run the platform test and existing port-ops/default-name tests. Expected:
all PASS; the broad boundary test remains RED because consumers have not moved.

- [ ] **Step 7: Checkpoint**

Record: `feat: add versioned BTstack platform descriptor`.

---

### Task 3: Route all common critical sections through system ops

**Files:**

- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/core/btstack_core_backend.c`
- Modify: `btstack/core/btstack_core_event_mapper.c`
- Modify: `btstack/core/btstack_event_queue.c`
- Modify: `btstack/core/btstack_property.c`
- Modify: `btstack/core/tests/test_btstack_platform.c`

- [ ] **Step 1: Extend host tests with critical-depth assertions**

The stub enter/exit callbacks maintain `critical_depth`, `enter_count`, and
`exit_count`. Exercise one state write/read, one property operation, and one
event enqueue/dequeue. Assert every protected mutation happens with depth 1,
the final depth is zero, and enter/exit counts match.

- [ ] **Step 2: Confirm RED against current common files**

Run the platform host test with the real selected common sources. Expected:
compile failure on `task.h` or a runtime assertion that platform critical ops
were not used.

- [ ] **Step 3: Replace common critical calls mechanically**

In the five listed production files:

```c
taskENTER_CRITICAL();
/* protected code */
taskEXIT_CRITICAL();
```

becomes:

```c
btstack_platform_enter_critical();
/* protected code */
btstack_platform_exit_critical();
```

Add `#include "port/btstack_platform.h"` and remove `task.h` wherever no other
FreeRTOS task primitive remains. Preserve early-return exit paths exactly;
never notify a worker while the common critical section is held.

- [ ] **Step 4: Bind the platform before the first protected init access**

At the start of `bt_xstack_init()`, after the null application-config check:

```c
if (btstack_platform_get() == NULL) {
    if (btstack_platform_activate(btstack_platform_default()) !=
            BTSTACK_PLATFORM_OK) {
        return BT_XSTACK_INTERNAL_ERROR;
    }
}
```

Application fields remain validated as before. Platform worker validation will
replace `configMAX_PRIORITIES` in Task 5; until then retain that check behind
the temporary GPCV2178 build.

- [ ] **Step 5: Verify**

Run platform, default-name, event-dispatch, PBAP runtime, and queue contracts.
Expected: PASS. Boundary remains RED only for queue/task/run-loop/transport
dependencies, not for `taskENTER_CRITICAL` in common code.

- [ ] **Step 6: Checkpoint**

Record: `refactor: route common locking through platform ops`.

---

### Task 4: Replace FreeRTOS queues with common fixed rings

**Files:**

- Create: `btstack/core/btstack_ring.h`
- Create: `btstack/core/btstack_ring.c`
- Create: `btstack/core/tests/test_btstack_ring.c`
- Create: `btstack/core/tests/test_btstack_ring.ps1`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/Makefile.inc`

- [ ] **Step 1: Write ring RED tests**

Use a three-item `uint32_t` ring and assert empty pop fails, three pushes
succeed, fourth push fails, FIFO order is exact across wraparound, and
`count/spaces` are correct. Compile with C99 `-Wall -Wextra -Werror` and no
RTOS stubs.

- [ ] **Step 2: Implement the private ring**

Use this exact state shape:

```c
typedef struct {
    uint8_t *storage;
    uint16_t item_size;
    uint16_t capacity;
    uint16_t read_index;
    uint16_t write_index;
    uint16_t count;
} btstack_ring_t;
```

`btstack_ring_push()` and `btstack_ring_pop()` perform one bounded `memcpy`,
advance modulo capacity, and return `false` on full/empty. They contain no lock;
the caller must hold the platform critical section.

- [ ] **Step 3: Replace three FreeRTOS queue objects**

Keep these capacities unchanged:

```c
BT_XSTACK_REQUEST_QUEUE_LEN             8u
BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN     16u
BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN  16u
```

Replace `StaticQueue_t`, `QueueHandle_t`, and byte-control storage with typed
static arrays plus three `btstack_ring_t` objects. Initialize them with
`btstack_ring_init()` in `bt_xstack_init()`.

- [ ] **Step 4: Preserve admission and reserve behavior**

All push/pop and `spaces <= BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE` decisions
must occur inside one platform critical section. Preserve return values:

- Public request full: `BT_XSTACK_QUEUE_FULL`.
- PBAP record backpressure: existing `BT_XSTACK_API_EVENT_BACKPRESSURE` value.
- Ordinary critical event rejected at reserve: increment critical drop.
- Best-effort full: increment best-effort drop.
- Control event may consume reserve and fails only when physically full.

Continue using the temporary FreeRTOS task notification solely to wake the
existing API/event tasks. The API task pops all available requests after wake;
the event task drains critical before best-effort exactly as today.

- [ ] **Step 5: Preserve reliable event-task request admission**

When the event worker must submit `PBAP_PEER_DISCONNECTED` or
`PBAP_EVENT_SPACE_AVAILABLE`, loop until the common request ring has space.
The API worker notifies the event worker after each request pop. No busy spin
and no new queue are permitted.

- [ ] **Step 6: Verify ring and behavior tests**

Run ring, phonebook runtime, PBAP queue, API queue, event-dispatch, and demo
contracts. Expected: PASS with the same queue counts and reserve behavior.

- [ ] **Step 7: Checkpoint**

Record: `refactor: own API and event rings in common core`.

---

### Task 5: Move API and event task scheduling into worker ops

**Files:**

- Create: `btstack/ports/gpcv2178/btstack_rtos_gpcv2178.c`
- Create: `btstack/ports/gpcv2178/btstack_rtos_gpcv2178.h`
- Create: `btstack/core/tests/test_btstack_workers.c`
- Create: `btstack/core/tests/test_btstack_workers.ps1`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/core/btstack_platform.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Modify: `btstack/ports/gpcv2178/btstack_config.h`

- [ ] **Step 1: Write worker-context RED tests**

The mock port stores three entry functions and signal masks without spawning
native tasks. Tests explicitly invoke the captured entries and assert:

- `bt_xstack_*` request admission does not call an application callback.
- API work runs only after invoking the API entry.
- Ordinary callbacks run only after invoking the EVENT entry.
- Audio delivery remains direct on the simulated STACK context.
- Notify observes `critical_depth == 0`.
- A full reliable request waits for `REQUEST_SPACE` and resumes after API pop.

- [ ] **Step 2: Confirm RED**

Compile real API core against the mock descriptor without FreeRTOS headers.
Expected: compile failure on `FreeRTOS.h`, `queue.h`, or `task.h`.

- [ ] **Step 3: Implement the GPCV2178 worker adapter**

Move API/event task control blocks, static stacks, and handles out of API core.
Keep exact stack sizes:

```c
BT_XSTACK_API_TASK_STACK_WORDS    2048u
BT_XSTACK_EVENT_TASK_STACK_WORDS  1024u
```

`validate_config()` preserves current public validation: API stack words may
be zero or 2048; a nonzero requested priority must be below
`configMAX_PRIORITIES`. Event uses the same normalized priority as API. STACK
uses existing `BTSTACK_GPCV2178_TASK_PRIORITY` and stack size.

Use `xTaskCreateStatic()` for API/event and the current stack task creation
policy for STACK. Map notify/wait to FreeRTOS notification bits using
`xTaskNotify(..., eSetBits)` and `xTaskNotifyWait()`. All FreeRTOS types and
macros remain inside this port file.

- [ ] **Step 4: Convert common worker entries**

Remove all FreeRTOS headers/types from `bt_xstack_api_core.c`. Start API and
EVENT through `platform->workers.start()`. Replace waits/notifies with the
platform wrappers and these signals:

```c
API:   WORK | COMMAND_SLOT
EVENT: WORK | REQUEST_SPACE
```

`btstack_core_command_slot_released()` notifies API with `COMMAND_SLOT`.
Request/event pushes notify the target worker with `WORK`. Request pops notify
EVENT with `REQUEST_SPACE`. Rings, not signal bits, are the source of truth:
each worker drains all currently available work before waiting again. This
prevents a `WORK` bit received during a `COMMAND_SLOT` wait from stranding a
request after the bit is consumed.

- [ ] **Step 5: Roll back partial worker start**

If API starts but EVENT fails, call `stop(API)` and return the existing
`BT_XSTACK_INTERNAL_ERROR`. No READY/power event may be emitted. Add a mock
failure test that checks stop order and cleared initializing state.

- [ ] **Step 6: Verify**

Run worker host test plus API runtime, API queue, event dispatch, PBAP queue,
and GPCV2178 lifecycle contracts. Expected: PASS. The common API source must
have zero FreeRTOS includes/calls.

- [ ] **Step 7: Checkpoint**

Record: `refactor: schedule API and event workers through port`.

---

### Task 6: Make runtime, run loop, and HCI initialization generic

**Files:**

- Create: `btstack/ports/gpcv2178/btstack_platform_gpcv2178.c`
- Create: `btstack/core/tests/test_btstack_runtime_platform.c`
- Create: `btstack/core/tests/test_btstack_runtime_platform.ps1`
- Modify: `btstack/core/btstack_runtime.c`
- Modify: `btstack/core/btstack_runtime.h`
- Modify: `btstack/core/btstack_core_backend.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.h`
- Modify: `btstack/ports/gpcv2178/btstack_run_loop_gpcv2178.h`

- [ ] **Step 1: Write lifecycle-order RED tests**

The mock descriptor records calls and must observe:

```text
activate
early_init
run_loop_init
memory_init
hci_init
hci_set_control
after_hci_init
l2cap_init
adapter_setup
classic_profiles_setup
core_backend_init
start(API)
start(EVENT)
```

`bt_xstack_enable()` must then record `start(STACK)` before asynchronous power
request admission. Inject failure at each initialization hook and assert only
completed stages unwind, no READY event appears, and init returns
`BT_XSTACK_INTERNAL_ERROR`.

- [ ] **Step 2: Confirm RED**

Compile `btstack_runtime.c` without GPCV2178 headers. Expected: failure on
`btstack_gpcv2178_port.h` or `btstack_run_loop_gpcv2178.h`.

- [ ] **Step 3: Build the GPCV2178 descriptor**

Move the existing immutable UART transport configuration and classic-profile
configuration into `btstack_platform_gpcv2178.c`. Implement descriptor getters
that select the current H4/H5/UART/SLIP combination using existing config
macros. Supply:

- Current system and storage callbacks.
- Worker ops from `btstack_rtos_gpcv2178.c`.
- Existing `btstack_control_gpcv2178`.
- GPCV2178 run-loop instance, callback enqueue, and diagnostics.
- The existing HCI diagnostic handler through `after_hci_init()`.

Export exactly one strong symbol:

```c
const btstack_platform_t *btstack_platform_default(void)
{
    return &btstack_platform_gpcv2178;
}
```

- [ ] **Step 4: Move generic initialization to runtime**

`btstack_runtime_init()` must obtain the activated descriptor and perform the
existing sequence with generic getters:

```c
btstack_run_loop_init(platform->stack.get_run_loop(context));
btstack_memory_init();
hci_init(platform->stack.get_hci_transport(context),
         platform->stack.get_hci_transport_config(context));
hci_set_control(platform->controller.control);
platform->stack.after_hci_init(context);
l2cap_init();
btstack_adapter_setup();
btstack_classic_profiles_setup(
    platform->stack.get_classic_profiles_config(context));
btstack_core_backend_init();
```

Retain current logging as optional platform log calls. Do not move application
callbacks or queue creation into runtime.

- [ ] **Step 5: Generic stack start and callback posting**

`btstack_runtime_start()` starts `BTSTACK_WORKER_STACK` with a common entry that
calls `btstack_run_loop_execute()`. `btstack_runtime_started()` uses common
lifecycle state. `btstack_core_backend.c` replaces
`btstack_run_loop_gpcv2178_enqueue_callback()` with
`btstack_platform_post_stack_callback()`.

- [ ] **Step 6: Preserve diagnostics**

Map `btstack_stack_queue_diagnostics_t` into the unchanged public/internal
runtime diagnostic structure. Verify current depth, maximum depth, and full
count remain available.

- [ ] **Step 7: Verify**

Run runtime-platform, worker, backend, event-dispatch, GPCV2178 lifecycle,
AVRCP scheduler, PBAP, and SCO contracts. Expected: PASS. `btstack/core` must
have zero GPCV2178 identifiers/includes.

- [ ] **Step 8: Checkpoint**

Record: `refactor: bind runtime and HCI through platform descriptor`.

---

### Task 7: Clean the generic transport boundary

**Files:**

- Create: `btstack/transport/tests/test_btstack_transport_boundary.ps1`
- Create: `btstack/transport/tests/test_btstack_transport_host.c`
- Create: `btstack/transport/tests/test_btstack_transport_host.ps1`
- Modify: `btstack/transport/hci_transport_h4.c`
- Modify: `btstack/transport/hci_transport_h5.c`
- Modify: `btstack/transport/btstack_uart_block.h`
- Modify: `btstack/transport/Makefile.inc`

- [ ] **Step 1: Write transport RED tests**

The static test rejects vendor/RTOS includes and the concrete symbols:

```text
btstack_uart_block_windows_instance
btstack_uart_block_embedded_instance
btstack_uart_block_freertos_instance
```

The C fixture compiles H4, H5, SLIP, and UART wrapper headers/implementations
against mock UART/run-loop functions with C99 `-Wall -Wextra -Werror`.

- [ ] **Step 2: Confirm RED**

Expected: static failure on `gplib_print_string.h` and the three instance
declarations.

- [ ] **Step 3: Remove vendor logging includes**

Delete `#include "gplib_print_string.h"` from H4/H5. Retain existing
`BTSTACK_PRINTF` call sites through `btstack_debug.h`; do not change diagnostic
conditions or payload logging policy.

- [ ] **Step 4: Move concrete instance declarations to their owners**

Remove the three instance functions from `btstack_uart_block.h`. If a selected
port still consumes one, declare it in that port's private header. The generic
header retains only `btstack_uart_block_t` and common function-table contracts.

- [ ] **Step 5: Verify transport and full boundary**

Run both transport tests and `test_btstack_platform_boundary.ps1`. Expected:
PASS with zero platform/RTOS hits in all five common production areas.

- [ ] **Step 6: Checkpoint**

Record: `refactor: make BTstack transport platform neutral`.

---

### Task 8: Add the application-owned audio boundary and HFP uplink ring

**Files:**

- Create: `btstack/core/btstack_call_audio_tx.c`
- Create: `btstack/core/btstack_call_audio_tx.h`
- Create: `btstack/api/tests/test_bt_xstack_call_audio.c`
- Create: `btstack/api/tests/test_bt_xstack_call_audio.ps1`
- Modify: `btstack/include/api/bt_xstack_audio.h`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/core/btstack_core_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/profiles/btstack_hfp_hf.c`
- Modify: `btstack/include/profiles/btstack_hfp_hf.h`
- Modify: `btstack/Makefile.inc`
- Modify: `Project/wifi_demo/bt_xstack_demo.c`

- [ ] **Step 1: Write public API and ring RED tests**

Compile the real public header from both C99 and C++11. Runtime tests assert:

```c
CHECK(bt_xstack_call_audio_write(NULL, 1u) ==
      BT_XSTACK_INVALID_ARGUMENT);
CHECK(bt_xstack_call_audio_write(data, 0u) ==
      BT_XSTACK_INVALID_ARGUMENT);
CHECK(bt_xstack_call_audio_write(data, 256u) ==
      BT_XSTACK_INVALID_ARGUMENT);
CHECK(bt_xstack_call_audio_write(data, 48u) ==
      BT_XSTACK_NOT_READY); /* no active SCO */
```

After establishing a mock SCO link with TX frame length 48, fill the 2048-byte
ring without partial admissions, assert the next complete write returns
`BT_XSTACK_QUEUE_FULL`, signal can-send-now repeatedly, and verify exact FIFO
payload/header output. Disconnect must clear unread bytes.

- [ ] **Step 2: Confirm RED**

Expected: compile failure because `bt_xstack_call_audio_write()` is absent.

- [ ] **Step 3: Extend the common HFP facade, not upstream BTstack**

Track `sco_handle` and `tx_packet_length` from
`HFP_SUBEVENT_AUDIO_CONNECTION_ESTABLISHED`; clear them on audio/SLC release.
Add internal facade functions:

```c
typedef void (*btstack_hfp_audio_tx_ready_callback_t)(void);

int btstack_hfp_hf_set_audio_tx_ready_callback(
    btstack_hfp_audio_tx_ready_callback_t callback);
uint16_t btstack_hfp_hf_get_audio_tx_frame_length(void);
int btstack_hfp_hf_request_audio_can_send_now(void);
int btstack_hfp_hf_send_audio_frame(
    const uint8_t *data, uint16_t length);
```

Register a normal HCI event handler for `HCI_EVENT_SCO_CAN_SEND_NOW`. The send
function requires the active handle, exact negotiated TX frame length,
`hci_can_send_sco_packet_now()`, reserves the HCI buffer, writes the three-byte
SCO header plus payload, and calls `hci_send_sco_packet_buffer()`.

- [ ] **Step 4: Implement the fixed common TX ring**

Use exactly:

```c
#define BT_XSTACK_CALL_AUDIO_TX_BYTES   2048u
#define BT_XSTACK_CALL_AUDIO_WRITE_MAX   255u
```

The ring is a byte ring protected by platform critical ops. Public writes are
all-or-nothing and never block. Request can-send-now only when at least one
complete negotiated frame is buffered. On ready, peek one complete frame into
a 255-byte staging buffer together with ring read index and connection
generation, release the critical section, and send it. Consume the peeked bytes
only when send succeeds and generation/read index still match. Disconnect
increments generation and clears the ring, so a synchronous disconnect or
session replacement cannot consume data from a new call. Request another ready
event if another complete frame remains. Never hold the critical section while
calling HCI.

- [ ] **Step 5: Preserve downlink callback behavior**

Do not enqueue A2DP/HFP payloads into the ordinary event ring. Keep
`media_audio_callback` and `call_audio_callback` direct from the BTstack worker,
with callback-lifetime-only pointers. Assert the platform port contains no
DAC/I2S/playback/capture callback. The existing demo continues copying data to
`bt_audio_task_write_media_pcm()` and `bt_audio_task_write_call_pcm()`.

- [ ] **Step 6: Verify audio behavior**

Run call-audio, SCO packet safety, demo audio, A2DP media, header compilation,
worker-context, and platform-boundary tests. Expected: PASS; no dynamic memory,
new task, or general queue.

- [ ] **Step 7: Checkpoint**

Record: `feat: add callback-only audio boundary and HFP uplink ring`.

---

### Task 9: Make every build select exactly one port

**Files:**

- Create: `btstack/ports/README.md`
- Modify: `btstack/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `tools/wireless_lib/wireless_sources.json`
- Modify: `btstack/api/tests/test_bt_xstack_pbap_build_package_contract.ps1`
- Modify: `btstack/core/tests/test_btstack_core_source_layout.ps1`
- Modify: `btstack/README.md`

- [ ] **Step 1: Strengthen build-selection contracts and obtain RED**

Require every common new source exactly once, require the two GPCV2178 port
sources exactly once in each GPCV2178-selected project, reject
`ports/gpcv2178` from common `BTSTACK_INCLUDE_DIRS`, and assert generated make
rules are unique. Also require exactly one definition of
`btstack_platform_default` in `libxstack_api.a`.

- [ ] **Step 2: Separate common and selected-port manifests**

`btstack/Makefile.inc` lists `include/port`, `btstack_platform.c`,
`btstack_ring.c`, and `btstack_call_audio_tx.c`, but no concrete port path.
Define a separate GPCV2178 source/include block in each selected project or a
port-local manifest. Do not add a runtime platform registry or weak fallback.

- [ ] **Step 3: Regenerate API and AT makefiles**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1 -Jobs 4
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Project\WirelessLibraries\BTstack_AT_Library\build_btstack_at.ps1 -Jobs 4
```

Expected: both exit 0; generated makefiles contain common sources once and
GPCV2178 port sources once.

- [ ] **Step 4: Document a new-port checklist**

`btstack/ports/README.md` must require:

1. One strong `btstack_platform_default()`.
2. Static or application-supplied worker memory; no common RTOS headers.
3. System/worker/run-loop/HCI/controller mandatory bindings.
4. Optional storage/address services with explicit null behavior.
5. No application audio playback/capture in the port.
6. Boundary, mock-port, library, and target-link verification.

- [ ] **Step 5: Verify package and symbols**

Use ARM `ar`/`nm` to assert one each of `btstack_platform.o`,
`btstack_ring.o`, `btstack_call_audio_tx.o`,
`btstack_platform_gpcv2178.o`, `btstack_rtos_gpcv2178.o`, and exactly one
`btstack_platform_default`. Verify all existing public symbols remain once and
the new `bt_xstack_call_audio_write` appears once.

- [ ] **Step 6: Checkpoint**

Record: `build: select BTstack platform through one port package`.

---

### Task 10: Full compatibility and target verification

**Files:**

- Modify only if a verification failure identifies an in-scope defect.

- [ ] **Step 1: Run every API and profile PowerShell test independently**

Enumerate scripts as a real array and run each in a fresh PowerShell process:

```powershell
$tests = @(
    Get-ChildItem btstack\api\tests -Filter *.ps1 -File
    Get-ChildItem btstack\profiles\tests -Filter *.ps1 -File
    Get-ChildItem btstack\core\tests -Filter *.ps1 -File
    Get-ChildItem btstack\transport\tests -Filter *.ps1 -File
)
foreach ($test in $tests) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $test.FullName
    if ($LASTEXITCODE -ne 0) { throw "FAILED: $($test.FullName)" }
}
```

Expected: zero failures. Report exact pass counts per directory.

- [ ] **Step 2: Fresh-build API and AT libraries**

Run both official build scripts with `-Jobs 4`. Expected: exit 0, archive newer
than all project sources/objects, staged public headers match source hashes,
and no new warning attributable to the refactor.

- [ ] **Step 3: Regenerate and force-build the main firmware**

From `Project/GPCV2178_Turnkey_SDK`, run `gpcmprj2make.exe`, then:

```powershell
mingw32-make.exe -f GPCV2178_turnkey_sdk_release_Debug.mk -B -j4
```

Expected: generator and linker exit 0. Separate pre-existing warnings from any
new platform/audio warning; new warnings must be fixed before completion.

- [ ] **Step 4: Run final boundary and ABI scans**

Assert:

- Zero RTOS/GPCV2178/vendor dependencies in common production directories.
- Exactly one selected platform descriptor in the archive and ELF.
- Existing public function/type names still exist with unchanged declarations.
- Existing event structure sizes and enum values match their baseline
  contracts.
- Queue constants remain 8/16/16 with reserve 4.
- No `malloc`, `calloc`, `realloc`, `free`, new common task, or new general
  queue was introduced.

- [ ] **Step 5: Record target-only caveats**

Do not claim new-platform portability or HFP uplink audio quality from host and
cross-build evidence alone. Record that another RTOS port, target-board HFP TX,
A2DP/HFP callback timing, queue saturation, disconnect, and controller-specific
SCO format still require hardware validation.

- [ ] **Step 6: Final checkpoint**

Record: `verify: complete cross-RTOS BTstack port migration`.
