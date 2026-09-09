# ATBM `wlan_lib` Customer Layout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish the two wireless archives under ATBM `wlan_lib` and expose only self-contained public WiFi API headers in the customer SDK.

**Architecture:** The two independent G+ projects write their archives to a shared ATBM `wlan_lib` directory. Public ABI definitions move behind a small `api/atbm_wifi_types.h` boundary, while internal driver headers remain available only in the internal SDK. The minimal package builder stages the active application dependency closure, then enforces the narrower ATBM customer layout.

**Tech Stack:** PowerShell 5, G+ IDE project XML, Generalplus ARM GCC 4.9.3, GNU `strip`/`ranlib`, C headers, unified patch.

---

### Task 1: Add failing layout and API contracts

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_library_projects.ps1`
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`

- [ ] Assert the WiFi and BTstack G+ outputs are
  `../../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi` and
  `../../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/btstack`.
- [ ] Assert the main project links each archive once from `wlan_lib`.
- [ ] Add a minimal-stage fixture whose ATBM dependency closure contains
  public and private headers, then assert only `api` and `wlan_lib` survive.
- [ ] Run both tests and confirm they fail because the old `lib` layout is
  still active.

### Task 2: Make the public headers self-contained

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_types.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_driver_api.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_api.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/os/include/atbm_type.h`
- Modify: `Project/wifi_demo/atbm_wifi_probe_demo.c`

- [ ] Put the existing public scalar typedefs, `ATBM_NULL`, station-capacity
  constant, and opaque network declarations in `atbm_wifi_types.h`.
- [ ] Include the public type header from both API headers.
- [ ] Reuse the public types from the internal type header without duplicate
  typedefs.
- [ ] Give the public API its own station-capacity constant so it does not
  depend on the private driver configuration macro.
- [ ] Remove `atbm_hal.h` from the application Demo.
- [ ] Compile smoke-test translation units that include each public header
  without internal headers.

### Task 3: Move archive outputs and links

**Files:**
- Modify: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`

- [ ] Change both G+ project output paths to `atbm6165/wlan_lib`.
- [ ] Change the main Debug project archive paths to `atbm6165/wlan_lib`.
- [ ] Update minimal staging to strip the new archive paths.
- [ ] Enforce that the staged `atbm6165` directory contains only `api` and
  `wlan_lib`.
- [ ] Run the contract tests and confirm they pass.

### Task 4: Rebuild, package, and validate

**Files:**
- Modify: `docs/wireless_static_library_integration.md`
- Modify: `patches/wireless_static_library_integration.patch`
- Regenerate: sibling customer SDK

- [ ] Rebuild both libraries with the Debug baseline into `wlan_lib`.
- [ ] Run project and tool contract tests serially.
- [ ] Move the existing customer SDK to a timestamped backup and regenerate
  the official sibling customer SDK.
- [ ] Run the minimal customer compile/link verifier.
- [ ] Verify archive members, indexes, required symbols, checksum manifest,
  and absence of wireless implementation sources.
- [ ] Regenerate the existing-file patch from captured baselines and validate
  it against the modified files.
