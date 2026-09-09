# ATBM Full HAL WSM Bringup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the ATBM port from FW-only success to full HAL/WSM initialization while keeping the proven Generalplus SDIO bridge.

**Architecture:** Keep `WIFI_VENDOR=3`, `ATBM_GENERALPLUS_SDIO=1`, and the existing `atbm_os_sdio_gp.c` adapter. Disable `ATBM_SDIO_FW_ONLY` so `Atbmwifi_halEntry()` waits for normal WSM startup, then add the ATBM FreeRTOS driver source files needed by the full HAL and netstack path. Use compile/link checks as the automated gate and serial logs as the hardware gate.

**Tech Stack:** Generalplus GPCV2178 SDK project XML, ATBM RTOS WiFi6 C sources, FreeRTOS, ARM GCC.

---

### Task 1: Switch The Build Out Of FW-Only

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] Set Debug and Release compiler macros from `-DATBM_SDIO_FW_ONLY=1` to `-DATBM_SDIO_FW_ONLY=0`.
- [ ] Keep `-DATBM_SDIO_PROBE_ONLY=0` and `-DATBM_GENERALPLUS_SDIO=1`.
- [ ] Keep the FW-only helper in `atbm_main.c` compiled out by macro for future regression.

### Task 2: Add Full ATBM Driver Sources

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] Add ATBM upper HAL files used by the reference FreeRTOS project: `atbm_ap.c`, `atbm_config.c`, `atbm_etf.c`, `atbm_sta.c`, `atbm_txrx.c`, `atbm_wifi_driver_api.c`, `smartconfig.c`, and compression files.
- [ ] Add ATBM API file `api/atbm_api.c`.
- [ ] Add ATBM net files under `net/`.
- [ ] Add ATBM WPA/P2P/SAE/WPS files under `net/wpa/`.
- [ ] Do not add SPI, USB, test, or the reference `os/freertos/atbm_os_sdio.c`.
- [ ] Remove FW-only stub source from the active unit list when full HAL is enabled.

### Task 3: Compile And Identify Port Gaps

**Files:**
- Read/modify only the files reported by the compiler.

- [ ] Compile all active ATBM units with the project Debug flags.
- [ ] If a compile error is from unsupported reference OS glue, adapt only the smallest compatibility point.
- [ ] If a compile error is from unused optional features, disable the related ATBM config macro instead of pulling in unrelated subsystems.

### Task 4: Link And Preserve The Existing Vendor Switch

**Files:**
- Modify only ATBM adapter/config files if link reports ATBM symbols.

- [ ] Run a temporary full project link with rebuilt ATBM objects and project libraries.
- [ ] Fix ATBM-related undefined or duplicate symbols.
- [ ] Do not change unrelated resource/NVRAM/media errors.

### Task 5: Hardware Log Gate

**Expected serial milestones:**
- `FIRMWARE DOWNLOAD SUCCESS`
- IRQ registration succeeds.
- `WSM_STARTUP_IND` or equivalent firmware startup handling reaches `firmwareReady=1`.
- `ATBM_SDIO_FW_ONLY passed` no longer appears because full HAL path is active.
- Full HAL proceeds to MAC/efuse/netstack/interface initialization or prints the next concrete failing point.

