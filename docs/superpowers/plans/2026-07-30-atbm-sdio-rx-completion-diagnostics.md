# ATBM SDIO RX Completion Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add diagnostic-only evidence for RX worker completion, interrupt re-enable, and idle SDIO controller/CCCR state.

**Architecture:** Extend the existing AP RX diagnostic counters in the ATBM
common structure, update them at each RX worker boundary, and expose them
through `[APDIAG]`. Add completion and idle snapshots in the Generalplus SDIO
adapter without forcing recovery or changing disconnect policy.

**Tech Stack:** C, FreeRTOS/CMSIS-RTOS, Generalplus SD2.0 controller driver,
ATBM6165 SDIO HMAC, PowerShell source checks, ARM GCC 4.9.3.

---

### Task 1: Extend the diagnostic source check

**Files:**
- Modify: `tests/atbm/check_disconnect_diag.ps1`

- [ ] Add checks for RX worker counters and the three new markers.
- [ ] Run the check and verify it fails because scheme B is not implemented.

### Task 2: Add RX worker stage and completion counters

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/sdio/atbm_sdio.c`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_ap.c`

- [ ] Add entry, exit, re-enable, stage, and last-return fields.
- [ ] Set the stage before every potentially blocking RX operation.
- [ ] Print `[SDIO-RX-TASK]` after successful re-enable.
- [ ] Add the counters to `[APDIAG]`.

### Task 3: Add callback completion and idle snapshots

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c`

- [ ] Print `[SDIO-THREAD-DONE]` after the ATBM callback returns.
- [ ] Use a five-second queue timeout to print `[SDIO-IDLE]`.
- [ ] Read Controller IntEn/pending and CCCR INTx without clearing or handling
      the interrupt.

### Task 4: Verify and build

**Files:**
- Verify: `tests/atbm/check_disconnect_diag.ps1`
- Build: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] Run the source check and verify it passes.
- [ ] Compile all 96 ATBM WiFi objects.
- [ ] Relink 309 project objects and 19 archives with BTstack disabled.
- [ ] Verify all scheme B markers in the archive, ELF, and BIN.
- [ ] Record final sizes, timestamps, and SHA256 hashes.
