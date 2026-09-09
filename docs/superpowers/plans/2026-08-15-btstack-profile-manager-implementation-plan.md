# BTstack Profile Manager Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Classic-only setup module with a common Profile Manager that owns Classic lifecycle, aggregate state, event routing, and HFP-to-PBAP coordination while preserving API/AT behavior.

**Architecture:** Individual profiles keep native subscriptions and protocol state machines. `btstack_profile_manager` composes Classic profiles and routes normalized events through stateless Classic mapper functions. API and AT retain their existing reliable action transports.

**Tech Stack:** C99, PowerShell contracts, MinGW host fixtures, ARM GCC/G++, G+ `.gpcmprj` projects, static archives.

---

### Task 1: Profile Manager lifecycle and Classic composition

**Files:**
- Create: `btstack/core/btstack_profile_manager.h`
- Create: `btstack/core/btstack_profile_manager.c`
- Create: `btstack/core/tests/test_btstack_profile_manager.c`
- Create: `btstack/core/tests/test_btstack_profile_manager.ps1`

- [ ] **Step 1: Write the failing lifecycle test**

Define stub counters for `sdp_init`, `rfcomm_init`, PBAP, A2DP, AVRCP, and HFP setup. Assert NULL/short-size/nonzero-LE-mask configurations fail before any counter changes; a valid Classic ALL config calls each setup exactly once in order; repeated identical init is OK; setup failure stops later calls.

- [ ] **Step 2: Run the lifecycle test and verify RED**

Run: `powershell.exe -NoProfile -File btstack/core/tests/test_btstack_profile_manager.ps1`

Expected: compile failure because `btstack_profile_manager.h/.c` do not exist.

- [ ] **Step 3: Implement the manager config and lifecycle**

Add `btstack_profile_manager_config_t`, explicit Classic mask constants, reserved `le_profile_mask`, lifecycle guards, pre-validation, and the existing Classic setup order. Use fixed storage and `btstack_platform_enter_critical/exit_critical`; perform profile setup outside critical sections.

- [ ] **Step 4: Run the lifecycle test and verify GREEN**

Run the Task 1 script. Expected: all lifecycle/order/failure groups PASS under C99 `-Wall -Wextra -Werror`.

### Task 2: Replace Classic runtime/platform boundary

**Files:**
- Modify: `btstack/core/btstack_runtime.c`
- Modify: `btstack/include/port/btstack_stack_ops.h`
- Modify: `btstack/ports/gpcv2178/btstack_platform_gpcv2178.c`
- Delete: `btstack/profiles/btstack_classic_profiles.c`
- Delete: `btstack/include/profiles/btstack_classic_profiles.h`
- Modify: `btstack/core/tests/test_btstack_runtime_platform.c`
- Modify: `btstack/core/tests/test_btstack_platform.c`

- [ ] **Step 1: Update tests to require the manager boundary**

Replace `get_classic_profiles_config`/`btstack_classic_profiles_setup` stubs with `get_profile_manager_config`/`btstack_profile_manager_init`; assert runtime order remains adapter -> profile manager -> backend and manager failure enters the existing late-deinit path.

- [ ] **Step 2: Run runtime/platform tests and verify RED**

Run the runtime and platform scripts. Expected: failure on missing new stack-ops member and old runtime call.

- [ ] **Step 3: Migrate runtime and selected platform**

Change stack ops to return the manager config, change runtime to call manager init, migrate the GPCV2178 constant config, and remove both Classic composition files. Do not add LE setup calls.

- [ ] **Step 4: Run runtime/platform/manager tests and verify GREEN**

Expected: lifecycle order, reentry, diagnostics, and platform validation remain PASS.

### Task 3: Route normalized Classic events through Manager

**Files:**
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/core/btstack_profile_manager.h`
- Modify: `btstack/core/btstack_core_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Create: `btstack/core/tests/test_btstack_profile_manager_events.c`
- Create: `btstack/core/tests/test_btstack_profile_manager_events.ps1`

- [ ] **Step 1: Write failing event-routing tests**

Feed normalized HFP, A2DP, AVRCP, and PBAP events into manager handlers. Capture `btstack_core_post_event`; assert domain/id/address/payload mapping, one post per public event, no mapper-owned callback registration, and PBAP CONTACT/CALL returns zero only on post backpressure.

- [ ] **Step 2: Run and verify RED**

Expected: missing manager handlers and current mapper-owned subscriptions.

- [ ] **Step 3: Extract stateless Classic mapping functions**

Expose `btstack_event_mapper_map_hfp/a2dp/avrcp/pbap(source, destination)`. Move callback wiring and post decisions to Manager. Leave GAP/service mapping in its current transitional location. Keep direct audio bridges outside Manager.

- [ ] **Step 4: Run event-routing and existing profile tests**

Expected: new tests and HFP/A2DP/AVRCP/PBAP profile tests PASS.

### Task 4: Move aggregate Classic state ownership

**Files:**
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/core/btstack_profile_manager.h`
- Modify: `btstack/core/btstack_core_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/api/tests/test_bt_xstack_api_state_snapshot_contract.ps1`

- [ ] **Step 1: Add failing aggregate-state assertions**

Assert Manager updates HFP state/audio/voice, A2DP state, AVRCP state/play/volume, and PBAP state; command-side HFP/A2DP/AVRCP settings update Manager; public state copy overlays Manager Classic fields; Classic mapper functions contain no static state writes.

- [ ] **Step 2: Run and verify RED**

Expected: existing Classic state still lives in `btstack_core_event_mapper.c`.

- [ ] **Step 3: Implement Manager snapshots**

Store fixed HFP/A2DP/AVRCP/PBAP aggregate snapshots in Manager. Add short-critical read/write functions. Redirect Classic state reads and command-side state writes. Keep GAP/service/paired-device state transitional in the existing store and compose the public `bt_xstack_state_t` copy from both owners.

- [ ] **Step 4: Run state and API contracts**

Expected: state snapshot, backend, API queue, and public API contracts PASS.

### Task 5: Centralize HFP-to-PBAP rule

**Files:**
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/core/btstack_profile_manager.h`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/at/btstack_at_core_adapter.c`
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: relevant API/AT PBAP behavior tests

- [ ] **Step 1: Write failing output-before-action tests**

Assert HFP LOST first reaches the application callback or AT formatter, then `btstack_profile_manager_event_delivered` selects exactly one PBAP peer-disconnected action with the same address. Cover API request-ring saturation and AT stack-ingress saturation/retry without polling.

- [ ] **Step 2: Run and verify RED**

Expected: duplicated API/AT rule selection remains outside Manager.

- [ ] **Step 3: Implement Manager action selection**

Add a frontend-neutral fixed action type and registered submit sink. Move only the HFP LOST -> PBAP action decision into Manager. Keep API reliable request-ring and AT pending/inflight/STACK_INGRESS_SPACE transport mechanics in their adapters.

- [ ] **Step 4: Run API/AT PBAP behavior tests**

Expected: callback/text-before-cleanup, three serialized LOST events, same-address reuse, ingress-space retry, and idle/foreign PBAP safety PASS.

### Task 6: Build manifests, deletion contracts, and documentation

**Files:**
- Modify: `btstack/Makefile.inc`
- Modify: API/AT/Legacy `.gpcmprj` files and generated Debug makefiles
- Modify: wireless source manifest/generator if it owns Legacy units
- Modify: `btstack/README.md`
- Modify: layout/build/package contracts

- [ ] **Step 1: Add failing source-boundary contracts**

Require manager source exactly once in each selected project, old Classic source/header/symbol zero times, no LE setup call, and source-set equality with generated rules.

- [ ] **Step 2: Run and verify RED**

Expected: old Classic source still selected and manager missing.

- [ ] **Step 3: Update all source manifests and docs**

Replace the old unit with manager, regenerate official makefiles, document ownership and the zero LE mask rule, and keep frontend/source-set isolation unchanged.

- [ ] **Step 4: Run focused contracts and official builds**

Run all manager/runtime/profile/API/AT/layout/build/package scripts, then official API and AT builds and Legacy generator + forced build. Expected: zero errors and only known upstream warnings.

- [ ] **Step 5: Verify final artifacts**

Use ARM `ar`/`nm` to assert manager object/symbol exactly once, old Classic object/symbol absent, public ABI unchanged, headers synchronized, archives newer than sources/objects, and final API ELF still contains no AT resources.
