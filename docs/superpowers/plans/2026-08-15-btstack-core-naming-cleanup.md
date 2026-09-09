# BTstack Core Naming Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove stale API-specific names from the shared Core boundary and remove the redundant `core` segment from the two Core source filenames without changing public ABI or runtime behavior.

**Architecture:** API and AT remain separate frontends. Their shared request, state, diagnostics, and event-envelope boundary uses `btstack_core_*` names; API-private implementation helpers in `btstack/api/bt_xstack_api_core.c` keep `bt_xstack_api_*`. Global Core symbols retain the `btstack_core_*` namespace because C linkage has no directory namespace.

**Tech Stack:** C99, PowerShell contract tests, GPC project generator, ARM GCC/binutils.

---

### Task 1: Lock the intended naming boundary

**Files:**
- Create: `btstack/core/tests/test_btstack_core_naming.ps1`

- [ ] Add a contract requiring `btstack/core/btstack_backend.c` and `btstack/core/btstack_event_mapper.c`, forbidding their old filenames, and forbidding `bt_xstack_api_*` identifiers in shared Core production files.
- [ ] Require representative shared names: `btstack_core_request_t`, `btstack_core_event_envelope_t`, `btstack_core_dispatch`, `btstack_core_state_read_hfp`, and `btstack_frontend_deliver_event`.
- [ ] Run `powershell.exe -NoProfile -File btstack/core/tests/test_btstack_core_naming.ps1` and verify it fails because the old files and identifiers still exist.

### Task 2: Rename shared Core files and identifiers

**Files:**
- Move: `btstack/core/btstack_core_backend.c` to `btstack/core/btstack_backend.c`
- Move: `btstack/core/btstack_core_event_mapper.c` to `btstack/core/btstack_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/at/btstack_at_core_adapter.c`
- Modify: affected C fixtures under `btstack/api/tests`, `btstack/at/tests`, and `btstack/core/tests`

- [ ] Rename shared request/snapshot/diagnostic/event-envelope types from `bt_xstack_api_*` to `btstack_core_*`.
- [ ] Rename shared backend/state/event functions from `bt_xstack_api_*` to `btstack_core_*`.
- [ ] Rename the frontend event sink to `btstack_frontend_deliver_event`.
- [ ] Keep public `bt_xstack_*` functions and API-private static `bt_xstack_api_*` helpers unchanged.
- [ ] Run the naming contract and focused API/AT/Profile Manager tests; verify they pass.

### Task 3: Update build manifests and generated projects

**Files:**
- Modify: `btstack/Makefile.inc`
- Modify: `tools/wireless_lib/wireless_sources.json`
- Modify: three BTstack `.gpcmprj` projects and their generated Debug makefiles
- Modify: affected path-based PowerShell contracts

- [ ] Replace old source/object paths with `btstack_backend` and `btstack_event_mapper` in current build inputs and tests.
- [ ] Regenerate API, AT, and Legacy project makefiles using the existing official build flows.
- [ ] Verify each selected project contains each renamed Core source exactly once and contains neither old path.

### Task 4: Fresh verification

**Files:**
- Test: existing API, AT, Core, Profile, layout, package, and selected-build contracts

- [ ] Run focused naming/API/AT/Profile Manager contracts with zero failures.
- [ ] Fresh-build API, AT, and Legacy archives and force-link the main firmware.
- [ ] Use ARM `nm`/`ar` to verify old shared symbols are absent, new shared symbols occur once per applicable archive, and final public API symbols remain available.
