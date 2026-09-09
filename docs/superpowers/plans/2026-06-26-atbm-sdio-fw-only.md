# ATBM SDIO FW-Only Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an ATBM SDIO firmware-only bring-up stage that runs past SDIO probe, downloads firmware, waits for firmware ready, and stops before full WiFi netstack/interface initialization.

**Architecture:** Keep the existing vendor switch and ATBM SDIO bridge. Replace probe-only with a new `ATBM_SDIO_FW_ONLY` stage, then exit `Atbmwifi_halEntry()` immediately after firmware readiness checks.

**Tech Stack:** Generalplus G+ IDE project, FreeRTOS, ATBM RTOS WiFi6 SDIO HAL, GCC ARM embedded toolchain.

---

### Task 1: Add Stage Macro

**Files:**
- Modify: `atbm_rtos_wifi6/include/atbm_config.h`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] Verify current project has no `ATBM_SDIO_FW_ONLY` macro.
- [ ] Add `ATBM_SDIO_FW_ONLY` default value in ATBM config.
- [ ] Set `ATBM_SDIO_PROBE_ONLY=0` and `ATBM_SDIO_FW_ONLY=1` for Debug and Release targets.

### Task 2: Stop After Firmware Ready

**Files:**
- Modify: `atbm_rtos_wifi6/hal/atbm_main.c`

- [ ] Add a preprocessor branch after `atbm_firmware_init_check(hw_priv)`.
- [ ] Log `ATBM_SDIO_FW_ONLY passed`.
- [ ] Return before queue stats, netstack init, and interface creation.

### Task 3: Add Minimal Firmware Download Sources

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] Add ATBM SDIO firmware/hwio sources.
- [ ] Add ATBM HAL task/BH/timer/thread/WSM sources required for firmware ready.
- [ ] Add ATBM FreeRTOS OS abstraction sources required by the HAL.

### Task 4: Compile and Link Closure

**Files:**
- Modify additional ATBM/GP glue only if compile or link evidence requires it.

- [ ] Run static macro check and confirm the new stage is active.
- [ ] Compile newly-added source files with Debug target include/options.
- [ ] Link a temporary ELF with all current objects and existing libraries.
- [ ] Remove temporary link artifacts after verification.
