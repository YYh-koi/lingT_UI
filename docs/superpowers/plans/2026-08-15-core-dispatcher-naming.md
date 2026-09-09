# Core Dispatcher Naming Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename the Core request backend to the Core dispatcher while preserving public ABI and behavior.

**Architecture:** Keep the existing request execution and command-slot implementation intact. Change only the module filename, internal entry-point names, build metadata, and contracts; keep Event Mapper readiness owned and named by Event Mapper.

**Tech Stack:** C99, PowerShell contracts, GPC project generator, ARM GCC archives and firmware.

---

### Task 1: Add the naming contract

**Files:**
- Modify: `btstack/core/tests/test_btstack_core_naming.ps1`

- [ ] Require `btstack_core_dispatcher.c` and the approved internal names.
- [ ] Forbid `btstack_backend.c` and the old backend symbols.
- [ ] Run the contract and verify it fails because the new file is absent.

### Task 2: Rename the module and internal interfaces

**Files:**
- Move: `btstack/core/btstack_backend.c` to `btstack/core/btstack_core_dispatcher.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/core/btstack_runtime.c`
- Modify: `btstack/core/btstack_event_mapper.c`
- Modify: API/AT callers and focused fixtures under `btstack/api` and `btstack/at`

- [ ] Move the source file with no behavior change.
- [ ] Apply the approved dispatcher and Event Mapper symbol names.
- [ ] Run focused naming, API, AT, runtime, and diagnostics tests until green.

### Task 3: Update builds and verify artifacts

**Files:**
- Modify: `btstack/Makefile.inc`
- Modify: `tools/wireless_lib/wireless_sources.json`
- Modify: the API, AT, and Legacy `.gpcmprj` files and generated Debug makefiles

- [ ] Replace the source path in all current build metadata.
- [ ] Regenerate and rebuild API, AT, and Legacy archives.
- [ ] Force-link the main firmware.
- [ ] Verify new archive object/symbol names exist once and old names are absent.
- [ ] Run the complete PowerShell test matrix and report any independent baseline failure explicitly.

