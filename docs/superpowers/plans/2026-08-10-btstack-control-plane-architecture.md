# BTstack Control-Plane Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make xStack command ingress thread-safe and non-blocking, separate control-event delivery from commands, shorten synchronous Getter critical sections, and expose cached diagnostics without changing existing asynchronous API semantics.

**Architecture:** A GPCV2178-owned fixed callback ring is the only cross-task entry to the BTstack run loop. The API backend owns eight immutable prepared-command slots; a separate event task drains critical and best-effort queues, while cached state readers provide short domain snapshots.

**Tech Stack:** C11-compatible embedded C, BTstack run loop, FreeRTOS static queues/tasks/notifications, PowerShell contract tests, Generalplus ARM GCC build.

---

## File Responsibilities

- `btstack_api/ports/gpcv2178/btstack_run_loop_gpcv2178.c`: fixed callback ingress ring, wake-up, run-loop-side drain, port diagnostics.
- `btstack_api/ports/gpcv2178/btstack_run_loop_gpcv2178.h`: checked ingress and diagnostics declarations.
- `btstack_api/api/bt_xstack_api_backend.c`: prepared command-slot pool and minimal run-loop command callbacks.
- `btstack_api/api/bt_xstack_api_core.c`: request admission, command task, independent event task, public diagnostics aggregation, Getter routing.
- `btstack_api/api/bt_xstack_api_event.c`: cached state ownership, domain readers, event classification support.
- `btstack_api/api/bt_xstack_api_internal.h`: internal capacities, domain snapshots, backend diagnostics and task notification interfaces.
- `btstack_api/include/api/bt_xstack_types.h`: additive public diagnostics structure.
- `btstack_api/include/api/bt_xstack_service.h`: additive synchronous diagnostics Getter declaration.
- `btstack_api/api/tests/test_bt_xstack_run_loop_ingress_contract.ps1`: cross-task ring contract.
- `btstack_api/api/tests/test_bt_xstack_command_pool_contract.ps1`: non-blocking command-slot contract.
- `btstack_api/api/tests/test_bt_xstack_event_dispatch_contract.ps1`: independent event-task and loss-policy contract.
- `btstack_api/api/tests/test_bt_xstack_domain_getter_contract.ps1`: short cached-state Getter contract.
- `btstack_api/api/tests/test_bt_xstack_diagnostics_contract.ps1`: public diagnostics and recovery contract.

### Task 1: Thread-safe run-loop ingress

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_run_loop_ingress_contract.ps1`
- Modify: `btstack_api/ports/gpcv2178/btstack_run_loop_gpcv2178.c`
- Modify: `btstack_api/ports/gpcv2178/btstack_run_loop_gpcv2178.h`

- [ ] **Step 1: Write the failing contract test**

The test reads the port source/header and requires a fixed callback pointer
ring, read/write/count/max/full counters, duplicate suppression, critical
sections, a checked Boolean enqueue API, run-loop-side drain, and absence of
`btstack_run_loop_base_add_callback` inside the port's
`execute_on_main_thread` function.

```powershell
Assert-Match $source 'btstack_context_callback_registration_t\s*\*\s*btstack_run_loop_ingress\['
Assert-Match $source 'taskENTER_CRITICAL\s*\(\)'
Assert-Match $source 'btstack_run_loop_gpcv2178_drain_ingress\s*\('
Assert-NoMatch $executeBody 'btstack_run_loop_base_add_callback'
```

- [ ] **Step 2: Run the new test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\btstack_api\api\tests\test_bt_xstack_run_loop_ingress_contract.ps1
```

Expected: FAIL because the fixed ingress ring is absent and the current port directly calls `btstack_run_loop_base_add_callback`.

- [ ] **Step 3: Implement the fixed ingress ring**

Add a capacity-16 ring and this checked API:

```c
bool btstack_run_loop_gpcv2178_enqueue_callback(
    btstack_context_callback_registration_t *registration);

typedef struct {
    uint32_t current_depth;
    uint32_t max_depth;
    uint32_t full_count;
} btstack_run_loop_gpcv2178_diagnostics_t;

void btstack_run_loop_gpcv2178_get_diagnostics(
    btstack_run_loop_gpcv2178_diagnostics_t *diagnostics);
```

Enqueue and dequeue under short critical sections, suppress duplicate pointers,
wake after successful enqueue, and invoke callbacks outside the critical
section. Drain ingress before base callbacks/timers/data sources.

- [ ] **Step 4: Run the new test and existing run-loop/port contracts**

Expected: new test PASS; relevant GPCV2178 lifecycle/UART tests that target
`btstack_api` PASS.

### Task 2: Prepared command-slot pool and recoverable failures

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_command_pool_contract.ps1`
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`

- [ ] **Step 1: Write the failing command-pool contract**

Require eight slots, per-slot callback registration, checked port enqueue,
slot release notification, and removal of the single pending request,
completion queue, `BT_XSTACK_EXEC_TIMEOUT_MS`, and `bt_xstack_api_complete`.
Also require that stack-operation failure increments a counter rather than
calling `bt_xstack_api_mark_failed`.

- [ ] **Step 2: Run the command-pool test and verify RED**

Expected: FAIL on `bt_xstack_api_pending_request` and completion-wait code.

- [ ] **Step 3: Implement command slots**

Use:

```c
#define BT_XSTACK_COMMAND_SLOT_COUNT BT_XSTACK_REQUEST_QUEUE_LEN
#define BT_XSTACK_BACKEND_BUSY 1

typedef struct {
    bt_xstack_api_request_t request;
    btstack_context_callback_registration_t callback;
    uint8_t state;
} bt_xstack_api_command_slot_t;
```

The command task retries `BT_XSTACK_BACKEND_BUSY` after
`ulTaskNotifyTake(pdTRUE, portMAX_DELAY)`. The run-loop callback executes the
prepared request, records success/failure, releases the slot, and calls:

```c
void bt_xstack_api_command_slot_released(void);
```

Pure local requests return immediately without acquiring slots. Remove the
internal completion queue and global failure transition for individual command
failures.

- [ ] **Step 4: Run command-pool, API queue, and run-loop boundary tests**

Expected: PASS with no public completion event and preserved request ordering.

### Task 3: Independent notification-driven event task

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_event_dispatch_contract.ps1`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`

- [ ] **Step 1: Write the failing event-dispatch contract**

Require critical and best-effort static queues, a static event task, task
notification on successful post, scan-result classification as best effort,
critical-first draining, and removal of `BT_XSTACK_API_EVENT_POLL_MS`.

- [ ] **Step 2: Run the event-dispatch test and verify RED**

Expected: FAIL because one event queue is polled by the command task every 20 ms.

- [ ] **Step 3: Implement split queues and event task**

Use fixed capacities:

```c
#define BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN 16u
#define BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN 16u
#define BT_XSTACK_EVENT_TASK_STACK_WORDS 1024u
```

Classify only `BT_XSTACK_SERVICE_EVENT_SCAN_RESULT` as best effort. Notify the
event task after enqueue. The event task drains all critical events, then one
best-effort event, repeating so newly arrived critical events preempt scan
delivery. Control callbacks remain outside the run loop.

- [ ] **Step 4: Run event, API, Demo, and SCO contracts**

Expected: PASS; media/call PCM callbacks remain unchanged in this stage.

### Task 4: Domain readers and synchronous diagnostics

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_domain_getter_contract.ps1`
- Create: `btstack_api/api/tests/test_bt_xstack_diagnostics_contract.ps1`
- Modify: `btstack_api/api/bt_xstack_api_event.c`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`
- Modify: `btstack_api/include/api/bt_xstack_types.h`
- Modify: `btstack_api/include/api/bt_xstack_service.h`

- [ ] **Step 1: Write failing Getter and diagnostics contracts**

Require HFP/A2DP/AVRCP small-domain snapshot structures and reader functions.
Require individual Getters to call those readers and not call the full
`bt_xstack_api_snapshot`. Require an additive `bt_xstack_diagnostics_t` and
`bt_xstack_get_diagnostics()` with no request submission or run-loop entry.

- [ ] **Step 2: Run both tests and verify RED**

Expected: FAIL because individual Getters copy the full state and diagnostics
do not exist.

- [ ] **Step 3: Implement domain readers**

Define compact internal snapshots for service scalar, HFP, A2DP, and AVRCP
state. Copy only the requested domain under a critical section. Keep
`bt_xstack_get_state()` as the only whole-state copy path.

- [ ] **Step 4: Implement diagnostics aggregation**

Add:

```c
typedef struct {
    uint32_t accepted_requests;
    uint32_t rejected_requests;
    uint32_t stack_commands;
    uint32_t stack_command_failures;
    uint32_t command_slots_in_use;
    uint32_t command_slots_max;
    uint32_t run_loop_ingress_depth;
    uint32_t run_loop_ingress_max;
    uint32_t run_loop_ingress_full;
    uint32_t critical_event_drops;
    uint32_t best_effort_event_drops;
} bt_xstack_diagnostics_t;
```

`bt_xstack_get_diagnostics()` copies cached counters only and returns
`BT_XSTACK_NOT_INITIALIZED` before initialization.

- [ ] **Step 5: Run Getter, diagnostics, C/C++ header, and API surface tests**

Expected: PASS; no AT or command-completion symbol appears.

### Task 5: Full verification and artifacts

**Files:**
- Modify if required by generated project tooling: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk`
- Create: `tmp/btstack_control_plane_manifest.txt`

- [ ] **Step 1: Run all current API and Demo/SCO PowerShell tests**

Expected: all tests PASS with zero failures.

- [ ] **Step 2: Run relevant GPCV2178 port tests against `btstack_api`**

Tests with stale hard-coded `btstack/` paths are reported separately as an
existing test-fixture issue; new control-plane tests must pass.

- [ ] **Step 3: Build the API library**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1
```

Expected: exit 0 and updated `libxstack_api.a`.

- [ ] **Step 4: Generate and build the Debug application**

Run `gpcmprj2make.exe`, then `mingw32-make.exe -f GPCV2178_turnkey_sdk_release_Debug.mk -j4`.
Expected: exit 0 and updated `project.elf`.

- [ ] **Step 5: Verify symbols and save hashes**

Require the library to export `bt_xstack_get_diagnostics`, existing xStack
Getters, and backend dispatch. Require library/ELF absence of
`bt_xstack_send_at`. Save SHA-256 hashes for modified sources, tests, library,
and ELF in the manifest.
