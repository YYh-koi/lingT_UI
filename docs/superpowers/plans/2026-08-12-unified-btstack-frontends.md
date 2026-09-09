# Unified BTstack Frontends Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the separate legacy `btstack/` and typed `btstack_api/` snapshots with one current `btstack/` kernel and mutually exclusive API and AT application frontends.

**Architecture:** Neutral runtime, request, state, backend, and event-mapping code lives under `btstack/core/`. The API and AT frontends translate their inputs into the same internal requests and consume the same structured events. Port, GAP, and profile code never include either frontend, and each library project compiles the common manifest plus exactly one frontend.

**Tech Stack:** C99, FreeRTOS static queues/tasks, BTstack 1.6.2, SEGGER Embedded Studio project files, PowerShell contract tests.

---

### Task 1: Lock the dependency boundary with failing contracts

**Files:**
- Create: `tests/test_btstack_unified_layout.ps1`
- Create: `tests/test_btstack_frontend_build_separation.ps1`

- [ ] Assert that `btstack/core`, `btstack/api`, and `btstack/at` exist and `btstack_api` does not.
- [ ] Assert that `core`, `ports`, `gap`, `profiles`, and `adapter` do not include frontend-private headers.
- [ ] Assert that API and AT project source manifests share all common sources and include exactly one frontend.
- [ ] Run both scripts and observe the expected failure against the current split layout.

### Task 2: Establish the single source tree

**Files:**
- Replace: `btstack/` with the current contents of `btstack_api/`
- Delete after migration: `btstack_api/`
- Modify: all tests and project references that point at `btstack_api/`

- [ ] Verify the exact old and new directory paths before replacing the old tree.
- [ ] Move the current API snapshot into `btstack/` so PBAP and all current fixes become the baseline.
- [ ] Update path-only contracts and run the unified-layout test until the layout assertions pass.

### Task 3: Extract the neutral runtime core

**Files:**
- Create: `btstack/core/btstack_core_internal.h`
- Create: `btstack/core/btstack_runtime.c`
- Create: `btstack/core/btstack_request.c`
- Create: `btstack/core/btstack_state.c`
- Create: `btstack/core/btstack_backend.c`
- Create: `btstack/core/btstack_event_mapper.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Refactor from: `btstack/api/bt_xstack_api_core.c`
- Refactor from: `btstack/api/bt_xstack_api_backend.c`
- Refactor from: `btstack/api/bt_xstack_api_event.c`

- [ ] Add a failing dependency contract that rejects API/AT includes and initialization calls below the composition root.
- [ ] Move platform/profile initialization ownership into `btstack_runtime.c`.
- [ ] Move request queues and command slots into `btstack_request.c` and `btstack_backend.c`.
- [ ] Move state snapshots into `btstack_state.c`.
- [ ] Move native-event translation into `btstack_event_mapper.c` and publish through a neutral sink contract.
- [ ] Run the dependency and existing API contracts after every extraction step.

### Task 4: Reconnect the typed API frontend

**Files:**
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/api/bt_xstack_api_event.c`
- Modify: `btstack/api/bt_xstack_api_internal.h`
- Preserve: `btstack/include/api/*.h`
- Move/update: `btstack/api/tests/*`

- [ ] Add/retain failing contracts for every public API symbol and queue-admission behavior.
- [ ] Make public wrappers validate/copy arguments and submit neutral requests.
- [ ] Make the API event sink deliver structured callbacks without direct profile registration.
- [ ] Run API header, queue, event, PBAP, and behavior contracts.

### Task 5: Reconnect the AT frontend and add PBAP

**Files:**
- Modify: `btstack/at/btstack_at.c`
- Modify: `btstack/at/btstack_at_parser.c`
- Modify: `btstack/at/btstack_at_command_table.c`
- Create: `btstack/at/btstack_at_adapter.c`
- Modify: `btstack/at/btstack_at_protocol.c`
- Modify: `btstack/at/btstack_at_internal.h`
- Delete from build: `btstack/at/btstack_at_backend.c`
- Remove: `btstack/adapter/bt_xstack_manager.c`
- Add/update: `btstack/at/tests/*`

- [ ] Extend failing parser/table tests for `PBAPSYNC`, `PBAPCANCEL`, `PBAPAUTH`, and `PBAPSTAT`.
- [ ] Add failing formatter tests for `PBAPSTART`, `PBAPCONTACT`, `PBAPCALL`, `PBAPCATEND`, `PBAPDONE`, `PBAPAUTHREQ`, and `PBAPERROR`.
- [ ] Map all legacy commands and the four PBAP commands to neutral requests.
- [ ] Route command responses and structured asynchronous events through the single AT output path.
- [ ] Run exact-byte compatibility tests for the original 46 commands/events and the new PBAP protocol.

### Task 6: Build two mutually exclusive libraries

**Files:**
- Modify: `Project/WirelessLibraries/BTstack_API_Library/*`
- Create: `Project/WirelessLibraries/BTstack_AT_Library/*`
- Produce: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`
- Produce: `gplib/liba_wifi_src/atbm/atbm6165/bt_at/libxstack_at.a`

- [ ] Add a failing project contract for common-source parity and frontend exclusion.
- [ ] Update the API project to compile `btstack/` common sources plus `btstack/api/`.
- [ ] Create the AT project from the same common manifest plus `btstack/at/`.
- [ ] Build both from clean object directories.
- [ ] Check exported symbols and link an API harness and an AT harness independently.

### Task 7: Final verification

- [ ] Run all unified-layout and dependency tests.
- [ ] Run all API and AT contract/unit tests.
- [ ] Build both static libraries from clean state.
- [ ] Verify API/AT source-manifest separation and exported symbols.
- [ ] Record any board-only validation that remains: controller power, HFP, A2DP, AVRCP, PBAP, and PCM.

