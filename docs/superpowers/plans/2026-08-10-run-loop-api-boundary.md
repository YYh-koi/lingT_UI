# xStack API Run Loop Boundary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prepare application API requests outside the BTstack run loop and execute only BTstack-owned operations inside it.

**Architecture:** Keep the public `bt_xstack_*` API, request queue, internal completion token, cached synchronous Getters, and callback contract unchanged. Split the backend into an API-worker preparation/local-execution phase and a minimal run-loop stack-execution phase, with targeted state synchronization only for affected operations.

**Tech Stack:** C, FreeRTOS queues/critical sections, BTstack run loop callbacks, PowerShell structural contract tests, ARM GCC static-library and application builds.

---

### Task 1: Lock the run-loop boundary with a failing contract test

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_api_run_loop_boundary_contract.ps1`
- Read: `btstack_api/api/bt_xstack_api_backend.c`
- Read: `btstack_api/api/bt_xstack_api_core.c`

- [ ] **Step 1: Write the failing test**

Create a PowerShell contract that requires:

```powershell
$backend = Get-Content $backendPath -Raw
Assert-Contains $backend 'bt_xstack_api_execute_local_request'
Assert-Contains $backend 'bt_xstack_api_prepare_stack_request'
Assert-Contains $backend 'bt_xstack_api_execute_stack_request'
Assert-Contains $backend 'bt_xstack_api_complete(request->generation)'
Assert-Order $backend 'bt_xstack_api_prepare_stack_request' `
    'btstack_run_loop_execute_on_main_thread'
Assert-NotContains $runLoopCallback 'bt_xstack_api_state_sync_gap'
```

The test must also verify that the eight pure API request IDs occur in the local
request handler and not in the stack request handler, and that synchronous
AVRCP play-state Getter code uses `bt_xstack_api_snapshot` without
`bt_xstack_api_submit`.

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\btstack_api\api\tests\test_bt_xstack_api_run_loop_boundary_contract.ps1
```

Expected: FAIL because the three split backend functions do not exist.

### Task 2: Execute pure API state requests outside the run loop

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_run_loop_boundary_contract.ps1`

- [ ] **Step 1: Add a local-request classifier and executor**

Add a local handler for exactly these requests:

```c
BT_XSTACK_REQUEST_SET_PROFILE_MASK
BT_XSTACK_REQUEST_CLEAR_PEER_INDEX
BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS
BT_XSTACK_REQUEST_CLEAR_PEERS
BT_XSTACK_REQUEST_SET_TDL
BT_XSTACK_REQUEST_SET_HFP_CONFIG
BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE
BT_XSTACK_REQUEST_SET_A2DP_MUTE
```

The handler uses only the protected API state/event helpers. It returns a
classification result so unknown requests proceed to stack preparation. The
microphone-mute request preserves its active-call validation and event post.

- [ ] **Step 2: Complete local requests without scheduling the run loop**

In `bt_xstack_api_backend_dispatch()`, execute classified local requests and
post the existing internal generation completion immediately. Do not add a
public command-completion event.

- [ ] **Step 3: Run the focused test**

Expected: the local-request assertions pass while preparation assertions remain
RED until Task 3.

### Task 3: Prepare immutable stack requests in the API worker

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_run_loop_boundary_contract.ps1`

- [ ] **Step 1: Add stack-request preparation**

`bt_xstack_api_prepare_stack_request()` copies the queued request and performs:

```c
/* SET_NAME */ final text including the optional local-address suffix
/* CONNECT_LAST */ cached peer resolution into request.address
/* CONNECT */ profile-peer cache update before scheduling
/* HFP TOGGLE_AUDIO */ conversion to SET_AUDIO with an explicit boolean
/* AVRCP PLAY_PAUSE */ conversion to explicit PLAY or PAUSE
/* SCAN_START */ scan-filter copy into API event state
```

Preparation must fail before run-loop scheduling if cached state cannot resolve
the operation. All text and address data remain owned by the static prepared
request.

- [ ] **Step 2: Schedule only the prepared request**

Copy the prepared request into `bt_xstack_api_pending_request`, then call
`btstack_run_loop_execute_on_main_thread()`. The existing one-request-at-a-time
completion wait guarantees the static storage lifetime.

- [ ] **Step 3: Run the focused test**

Expected: preparation and ordering assertions pass.

### Task 4: Minimize the run-loop request executor

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_run_loop_boundary_contract.ps1`

- [ ] **Step 1: Replace the monolithic request function**

Rename/split the run-loop handler to
`bt_xstack_api_execute_stack_request()`. Remove pure API request cases and
remove name formatting, cached peer resolution, toggle selection, scan-filter
copy, and cached-state reads used only for preparation.

- [ ] **Step 2: Make GAP synchronization targeted**

Call `bt_xstack_api_state_sync_gap()` only after successful stack operations
that change GAP name, PIN, SSP, discoverability, connectability, class of
device, power, or scanning state. Remove the unconditional call from
`bt_xstack_api_execute()`.

- [ ] **Step 3: Preserve successful stack-backed settings**

After successful speaker-volume, AVRCP-config, repeat, or shuffle stack calls,
update their protected cached values as before. Connection/runtime states
remain event-driven.

- [ ] **Step 4: Run the focused test and verify GREEN**

Run the focused contract and require exit code 0.

### Task 5: Build and regression verification

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`
- Verify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Update: `tmp/run_loop_api_boundary_manifest.xml`

- [ ] **Step 1: Run all API contracts**

Run every `btstack_api/api/tests/test_*.ps1` and require zero failures.

- [ ] **Step 2: Build the independent API library**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1
```

Expected: exit code 0 and refreshed `libxstack_api.a`.

- [ ] **Step 3: Regenerate and build the Debug application**

Run `gpcmprj2make.exe` for the Debug target followed by
`mingw32-make.exe -j4`. Expected: exit code 0 and refreshed `project.elf`.

- [ ] **Step 4: Verify final ELF symbols**

Require `bt_xstack_init`, `bt_xstack_get_avrcp_play_state`, and
`btstack_run_loop_execute_on_main_thread`; reject `bt_xstack_send_at`.

- [ ] **Step 5: Save a SHA-256 manifest**

Export hashes for the modified backend, new contract, library, and ELF to
`tmp/run_loop_api_boundary_manifest.xml`.

This workspace has no Git metadata, so commit steps are intentionally omitted;
the SHA-256 manifest is the change checkpoint.
