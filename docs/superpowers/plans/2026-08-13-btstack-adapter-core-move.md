# BTstack Adapter Common Sources Move Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the platform-neutral adapter implementation and metadata out of `btstack/adapter`, delete that directory, and preserve behavior.

**Architecture:** Preserve all symbols and public headers. Only source locations and build metadata change; the GPCV2178 port continues to register the generic ops table.

**Tech Stack:** C99, PowerShell contracts, G+ `.gpcmprj`, generated GNU Make, ARM GCC.

---

### Task 1: Add the source-layout contract

**Files:**
- Create: `btstack/core/tests/test_btstack_core_source_layout.ps1`

- [x] Assert the three new `btstack/core/*.c` files exist and the old `btstack/adapter/*.c` paths do not.
- [x] Assert `bt_xstack_manager_internal.h` and stale `bt_xstack_manager.c` source references are absent.
- [x] Assert the common source manifest and wireless-library projects contain each new source path exactly once.
- [x] Run the contract and confirm it fails because the sources still live under `adapter`.

### Task 2: Move common sources and clean stale manager artifacts

**Files:**
- Move: `btstack/adapter/btstack_adapter.c` to `btstack/core/btstack_adapter.c`
- Move: `btstack/adapter/btstack_port_ops.c` to `btstack/core/btstack_port_ops.c`
- Move: `btstack/adapter/btstack_property.c` to `btstack/core/btstack_property.c`
- Delete: `btstack/adapter/bt_xstack_manager_internal.h`
- Delete: `btstack/adapter/tests/test_bt_xstack_manager_contract.ps1`
- Move: `btstack/adapter/Makefile.inc` to `btstack/Makefile.inc`
- Move: `btstack/adapter/README.md` to `btstack/README.md`
- Move: `btstack/adapter/tests/test_bt_xstack_build_integration.ps1` to `btstack/core/tests/test_bt_xstack_build_integration.ps1`
- Move: `btstack/adapter/tests/test_btstack_default_name.ps1` to `btstack/core/tests/test_btstack_default_name.ps1`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `tools/wireless_lib/wireless_sources.json`

- [x] Move implementation files without changing their APIs.
- [x] Replace source paths with `core/...`, remove stale manager entries, and remove obsolete adapter include paths.
- [x] Delete the empty `btstack/adapter` directory.
- [x] Run the new layout contract and confirm it passes.

### Task 3: Regenerate, build, and verify

**Files:**
- Regenerate: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library_Debug.mk`
- Regenerate: `Project/WirelessLibraries/BTstack_API_Library/obj/Debug/btstack/core/*.o`
- Regenerate: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`

- [x] Run `Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1 -Jobs 4`.
- [x] Regenerate and build the BTstack AT project and main firmware project.
- [x] Run the layout, common-layer build, API build/package, and focused API contracts.
- [x] Scan active build inputs for old adapter source/include paths and confirm none remain.
- [x] Confirm the archive contains the three moved object members and required symbols.
