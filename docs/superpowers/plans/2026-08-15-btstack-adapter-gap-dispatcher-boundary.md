# BTstack Adapter/GAP/Dispatcher Boundary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `btstack_adapter` the only common-layer GAP entry, reduce `btstack_core_dispatcher` to request scheduling/routing, and place single-profile and cross-profile behavior in their owning modules without changing public API/AT behavior.

**Architecture:** Keep the existing GAP implementation split into focused files and expose explicit adapter facade functions plus a compact adapter event callback. Route individual profile requests to profile-owned high-level operations and cross-profile requests to `btstack_profile_manager`. Preserve the fixed command slots, Stack Run Loop execution, worker/ring model, public ABI, and API/AT build separation.

**Tech Stack:** C99, BTstack 1.6.2, fixed-capacity rings/command slots, PowerShell contract tests, MinGW host tests, ARM GCC static-library builds.

---

### Task 1: Lock the Desired Dependency Boundary with Failing Tests

**Files:**
- Create: `btstack/core/tests/test_btstack_adapter_gap_boundary.ps1`
- Modify: `btstack/core/tests/test_btstack_core_naming.ps1`
- Modify: `btstack/api/tests/test_bt_xstack_api_backend_contract.ps1`

- [ ] **Step 1: Add a production include-boundary contract**

Create a test that enumerates production `.c/.h` files under `btstack/`, excludes `btstack/gap/**`, `btstack/core/btstack_adapter.c`, and tests, then rejects `#include "gap/..."`. It must additionally assert:

```powershell
Assert-Contains $adapterHeader 'btstack_adapter_set_power'
Assert-Contains $adapterHeader 'btstack_adapter_scan_start'
Assert-Contains $adapterHeader 'btstack_adapter_set_event_callback'
Assert-NotContains $dispatcher '#include "gap/'
Assert-NotContains $mapper '#include "gap/'
```

- [ ] **Step 2: Add dispatcher thinness assertions**

Require `btstack_core_dispatcher.c` to retain command-slot and platform-post symbols while forbidding direct policy symbols:

```powershell
Assert-Contains $dispatcher 'btstack_core_command_slot_acquire'
Assert-Contains $dispatcher 'btstack_platform_post_stack_callback'
Assert-NotContains $dispatcher 'btstack_gap_'
Assert-NotContains $dispatcher 'BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO'
Assert-NotContains $dispatcher 'BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE'
Assert-NotContains $dispatcher 'btstack_core_disconnect_all_profiles'
```

- [ ] **Step 3: Run the boundary tests and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_adapter_gap_boundary.ps1
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_core_naming.ps1
powershell.exe -NoProfile -File btstack/api/tests/test_bt_xstack_api_backend_contract.ps1
```

Expected: FAIL because dispatcher/event mapper still include GAP headers and contain GAP/Profile policies.

### Task 2: Add the Explicit Adapter Facade and Neutral Event Boundary

**Files:**
- Modify: `btstack/include/adapter/btstack_adapter.h`
- Modify: `btstack/core/btstack_adapter.c`
- Create: `btstack/core/tests/test_btstack_adapter.c`
- Create: `btstack/core/tests/test_btstack_adapter.ps1`

- [ ] **Step 1: Write the adapter behavior test**

Compile the real `btstack_adapter.c` against GAP/property/event stubs. Verify setup/deinit order, every facade method's exact argument/return forwarding, callback registration, and event conversion. Use a compact neutral event type whose payload is a union:

```c
typedef struct {
    btstack_adapter_event_type_t type;
    union {
        struct { bool working; } stack_state;
        struct { btstack_bd_addr_t address; } address;
        struct {
            btstack_bd_addr_t address;
            uint32_t class_of_device;
            int8_t rssi;
            uint8_t name_length;
            char name[BTSTACK_LOCAL_NAME_MAX];
        } scan_result;
        struct {
            btstack_bd_addr_t address;
            uint32_t numeric_value;
        } pairing_request;
        struct {
            btstack_bd_addr_t address;
            int status;
        } pairing_complete;
    } data;
} btstack_adapter_event_t;
```

- [ ] **Step 2: Run the adapter test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_adapter.ps1
```

Expected: compile failure because adapter operation/event interfaces do not exist.

- [ ] **Step 3: Add explicit adapter operations**

Declare and implement direct facade functions with existing BTstack status semantics:

```c
int btstack_adapter_set_power(bool enabled);
int btstack_adapter_set_name(const char *name);
int btstack_adapter_set_pin(const char *pin);
int btstack_adapter_set_ssp_mode(uint8_t mode);
int btstack_adapter_pairing_confirm(
    const btstack_bd_addr_t address, bool accept);
int btstack_adapter_set_discoverable(bool enabled);
int btstack_adapter_set_connectable(bool enabled);
int btstack_adapter_set_class_of_device(uint32_t class_of_device);
int btstack_adapter_scan_start(uint8_t duration);
int btstack_adapter_scan_stop(void);
int btstack_adapter_disconnect(void);
bool btstack_adapter_is_working(void);
int btstack_adapter_get_local_address(btstack_bd_addr_t address);
int btstack_adapter_get_state(btstack_adapter_state_t *state);
int btstack_adapter_set_event_callback(
    btstack_adapter_event_callback_t callback);
```

`btstack_adapter.c` remains a facade: each operation calls exactly one matching GAP function, and the GAP callback converts fields into the neutral union before invoking the registered adapter callback.

- [ ] **Step 4: Move existing GAP defaults into adapter setup**

After GAP initialization and callback binding, preserve existing defaults in this order:

```c
btstack_gap_set_pin("0000");
btstack_gap_set_ssp_mode(2u);
btstack_gap_set_connectable(true);
btstack_gap_set_discoverable(true);
```

Class of device and device name remain platform/profile-manager configuration values rather than hard-coded adapter policy.

- [ ] **Step 5: Run adapter and boundary tests**

Run the Task 2 behavior test and Task 1 boundary test. Expected: adapter behavior PASS; boundary remains RED only for consumers not yet migrated.

### Task 3: Route GAP State and Events Through Adapter

**Files:**
- Modify: `btstack/core/btstack_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/core/btstack_runtime.c`
- Modify: `btstack/core/tests/test_btstack_platform.c`
- Modify: `btstack/api/tests/test_bt_xstack_api_state_snapshot_contract.ps1`

- [ ] **Step 1: Extend event/state tests for adapter-only access**

Update host stubs to provide `btstack_adapter_get_state`, `btstack_adapter_set_discoverable`, and adapter callback registration. Feed each neutral adapter event and assert the existing public service/connection events and cached state are unchanged.

- [ ] **Step 2: Run focused tests and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_platform.ps1
powershell.exe -NoProfile -File btstack/api/tests/test_bt_xstack_api_state_snapshot_contract.ps1
```

Expected: compile/contract failure while event mapper still calls GAP directly.

- [ ] **Step 3: Replace event-mapper GAP dependencies**

Replace:

```c
btstack_gap_get_state(...)
btstack_gap_set_discoverable(...)
btstack_gap_set_event_callback(...)
```

with:

```c
btstack_adapter_get_state(...)
btstack_adapter_set_discoverable(...)
btstack_adapter_set_event_callback(btstack_core_adapter_event)
```

Translate the neutral union to the existing event envelope without changing filtering, paired-device tracking, drop behavior, or event-worker routing.

- [ ] **Step 4: Make runtime own mapper initialization**

Call `btstack_core_event_mapper_init()` from `btstack_runtime_init()` after adapter setup and before Profile Manager initialization. Remove mapper initialization from dispatcher initialization so dispatcher no longer owns unrelated component lifecycle.

- [ ] **Step 5: Run mapper, state, API/AT parity, and boundary tests**

Expected: all GAP consumers outside adapter are gone except the dispatcher, which is migrated in later tasks.

### Task 4: Move Cross-Profile Connection Operations into Profile Manager

**Files:**
- Modify: `btstack/include/core/btstack_profile_manager.h`
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/core/tests/test_btstack_profile_manager.c`
- Modify: `btstack/core/tests/test_btstack_profile_manager.ps1`

- [ ] **Step 1: Add failing Profile Manager operation tests**

Cover explicit address connect, connect-last peer resolution, one-profile disconnect, and disconnect-all ordering. Required public internal interfaces:

```c
int btstack_profile_manager_connect(
    bt_xstack_profile_t profile,
    const bt_xstack_addr_t *address);
int btstack_profile_manager_disconnect(bt_xstack_profile_t profile);
int btstack_profile_manager_disconnect_all(void);
```

Tests must verify disconnect-request state rollback when the profile call fails and that base ACL disconnect uses `btstack_adapter_disconnect()` after connected profiles are requested to disconnect.

- [ ] **Step 2: Run Profile Manager tests and verify RED**

Expected: link/compile failure for the missing operations.

- [ ] **Step 3: Implement connection routing in Profile Manager**

Use the existing peer-state helpers and existing HFP/A2DP/AVRCP connect/disconnect functions. Keep a direct `switch` and no registration table. `address == NULL` means resolve the stored peer; a non-NULL address is copied and cached before dispatch.

- [ ] **Step 4: Remove raw upstream GAP configuration from Profile Manager**

Replace direct `gap_set_local_name`, `gap_set_class_of_device`, and SSP setup calls with adapter facade calls. Preserve the configured device name and class of device. HFP gains remain owned by the HFP setup configuration.

- [ ] **Step 5: Run Profile Manager and profile-state tests**

Expected: all Profile Manager tests PASS with unchanged profile event/state behavior.

### Task 5: Move Single-Profile Request Semantics into HFP and AVRCP

**Files:**
- Modify: `btstack/include/profiles/btstack_hfp_hf.h`
- Modify: `btstack/profiles/btstack_hfp_hf.c`
- Modify: `btstack/include/profiles/btstack_avrcp.h`
- Modify: `btstack/profiles/btstack_avrcp.c`
- Modify: `btstack/profiles/tests/test_btstack_profile_contract.c`
- Modify: `btstack/profiles/tests/test_btstack_avrcp_default_policy.ps1`

- [ ] **Step 1: Add failing high-level HFP request tests**

Add a profile-owned command enum or explicit functions that cover answer, hang-up, multiparty choices, set/toggle audio, voice recognition, dial, redial, DTMF, and speaker gain. The HFP module must check its connection/audio state and map to its native controls.

- [ ] **Step 2: Add failing AVRCP play/pause tests**

Add a high-level control entry that accepts the public internal control value, maps play/pause based on AVRCP's own cached play status, rejects unknown state, and handles repeat/shuffle in AVRCP rather than dispatcher.

- [ ] **Step 3: Run profile tests and verify RED**

Run the HFP/profile contract and AVRCP default-policy tests. Expected: failure because the high-level entries are absent.

- [ ] **Step 4: Implement minimal profile-owned operations**

Keep the existing native low-level functions. Add thin high-level functions in each profile implementation; do not add persistent state outside the profile's existing state.

- [ ] **Step 5: Run all profile tests**

Expected: HFP, A2DP, AVRCP, PBAP and Profile Manager tests PASS.

### Task 6: Thin the Dispatcher and Preserve Request Semantics

**Files:**
- Modify: `btstack/core/btstack_core_dispatcher.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/api/tests/test_bt_xstack_api_backend_contract.ps1`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/core/tests/test_btstack_adapter_gap_boundary.ps1`

- [ ] **Step 1: Add exact route assertions**

Require GAP request IDs to call adapter operations, connect/disconnect IDs to call Profile Manager, and HFP/AVRCP IDs to call their profile high-level operations. Assert PBAP state synchronization and event-space completion behavior remain present.

- [ ] **Step 2: Run dispatcher contracts and verify RED**

Expected: failure because the old helper policies are still in dispatcher.

- [ ] **Step 3: Replace preparation policy with target-owned calls**

`btstack_core_prepare_stack_request()` must only deep-copy the request. Move name suffix behavior to `btstack_adapter_set_name` via an explicit suffix flag or dedicated adapter helper, connect-last resolution to Profile Manager, HFP toggle to HFP, AVRCP play/pause to AVRCP, and scan filtering to the event-mapper scan-filter setter at the scan operation boundary.

- [ ] **Step 4: Reduce stack execution to routing**

Delete dispatcher helpers for address conversion, disconnect-all, connect preparation, HFP mapping, and AVRCP mapping. Keep one request `switch` whose branches call adapter/Profile/Profile Manager and update only the existing shared snapshots/completion hooks required by the public API.

- [ ] **Step 5: Reduce dispatcher initialization**

`btstack_core_dispatcher_init()` resets only command slots and diagnostics counters. It must not initialize event mapper, set GAP defaults, or set HFP defaults.

- [ ] **Step 6: Run dispatcher, API, AT, and dependency contracts**

Expected: all Task 1–6 focused tests PASS and no production file outside adapter/GAP contains `#include "gap/`.

### Task 7: Build Integration and Full Verification

**Files:**
- Modify if generated source lists change: `btstack/Makefile.inc`
- Regenerate: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library_Debug.mk`
- Regenerate: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library_Debug.mk`
- Regenerate: `Project/WirelessLibraries/BTstack_Library/BTstack_Library_Debug.mk`

- [ ] **Step 1: Run all focused PowerShell tests**

Enumerate and execute each `*.ps1` under `btstack/api/tests`, `btstack/profiles/tests`, `btstack/core/tests`, `btstack/transport/tests`, `btstack/gap/tests`, and `btstack/at/tests` in a fresh PowerShell process. Record the first failure and do not hide the existing missing-patch baseline if it remains.

- [ ] **Step 2: Build all three static-library variants**

Run the official API and AT build scripts with `-Jobs 4`, regenerate/build Legacy with `-B -j4`, and verify zero new adapter/GAP/dispatcher warnings.

- [ ] **Step 3: Audit archive contents and public ABI**

Use ARM `ar`/`nm` to verify each selected archive contains exactly one adapter, dispatcher, event mapper and Profile Manager object/definition; API/AT frontend separation and static worker-resource separation remain unchanged. Verify packaged public header hashes match their source headers.

- [ ] **Step 4: Rebuild the main firmware**

Regenerate and force-link the main project, then verify the ELF is newer than the API archive and contains exactly one selected implementation with no AT resources in the API firmware.

- [ ] **Step 5: Final boundary scan**

Verify no new queue/task/heap allocation, no direct GAP dependency outside adapter/GAP, no removed public `bt_xstack_*` symbol, and no dispatcher-owned Profile policy.

## Plan Self-Review

- Spec coverage: adapter facade, GAP privacy, dispatcher thinness, Profile ownership, Profile Manager linkage, event flow, error/concurrency, memory and three build variants all have explicit tasks.
- Placeholder scan: no TBD/TODO or unspecified implementation step remains.
- Type consistency: adapter event/state types are introduced before mapper migration; Profile Manager operations are introduced before dispatcher routes to them; profile high-level operations are introduced before dispatcher uses them.
- Repository note: this workspace is not a Git repository, so the plan intentionally omits commit steps that cannot be performed here.
