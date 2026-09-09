# ATBM Generalplus SDIO Default Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the ATBM6165 driver always select the Generalplus SDIO port without requiring `-DATBM_GENERALPLUS_SDIO=1` in application or library projects.

**Architecture:** Force `ATBM_GENERALPLUS_SDIO` to `1` inside every ATBM platform header that currently supplies an independent fallback, then remove the redundant command-line definition from the Generalplus project files. Keep all existing conditional compilation sites and runtime SDIO logic unchanged.

**Tech Stack:** C preprocessor, Generalplus G+ project XML/makefiles, PowerShell contract tests, ARM GCC static-library build.

---

### Task 1: Add the configuration contract test

**Files:**
- Create: `tools/wireless_lib/tests/test_atbm_generalplus_sdio_default.ps1`

- [ ] Add a PowerShell test that requires the four ATBM platform headers to undefine any incoming value and then define `ATBM_GENERALPLUS_SDIO` as `1`, and rejects `-DATBM_GENERALPLUS_SDIO` from project compiler options.
- [ ] Run the test before implementation and verify it fails because the headers currently default to `0` and projects still pass the macro.

### Task 2: Make Generalplus SDIO the driver default

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/os/freertos/include/atbm_os_mem.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/os/freertos/include/atbm_os_sdio.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/os/freertos/include/os_api.h`

- [ ] Replace each independent fallback block with an `#ifdef`/`#undef` followed by `#define ATBM_GENERALPLUS_SDIO 1`.
- [ ] Verify an external `-DATBM_GENERALPLUS_SDIO=0` cannot change the selected platform path.

### Task 3: Remove redundant project macros

**Files:**
- Modify: Generalplus `.gpcmprj` files under `Project/GPCV2178_Turnkey_SDK` and `Project/WirelessLibraries` that contain `-DATBM_GENERALPLUS_SDIO=1`.
- Modify: generated Generalplus `.mk` files under the same project directories that contain the option.

- [ ] Remove only the `-DATBM_GENERALPLUS_SDIO=1` compiler option; do not change other options.
- [ ] Re-run the new contract test and existing wireless project tests.

### Task 4: Rebuild and publish the WiFi library

**Files:**
- Replace build output: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`

- [ ] Build the WiFi archive using `tools/wireless_lib/build_wireless_libs.ps1` without the project macro.
- [ ] Verify the build reports 96 WiFi objects and exits successfully.
- [ ] Run the focused ATBM SDIO/driver tests and record the final library SHA-256.
