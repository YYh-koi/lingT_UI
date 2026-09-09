# ATBM SDIO Disconnect Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add bounded diagnostics that identify the first layer losing a phone disconnect frame between the Generalplus SDIO controller and ATBM management-frame handling.

**Architecture:** Reuse the existing ATBM AP diagnostic counters and add two bounded trace points in the Generalplus-specific SDIO adapter. The change is observational only and retains the current interrupt and receive behavior.

**Tech Stack:** C, CMSIS-RTOS/FreeRTOS, Generalplus SDIO L1/L2, ATBM WSM driver, PowerShell verification.

---

### Task 1: Add a source-level diagnostic regression check

**Files:**
- Create: `tests/atbm/check_disconnect_diag.ps1`

- [ ] **Step 1: Write the failing check**

The script must require `ATBM_AP_RX_DIAG=1`, `ATBM_AP_LINK_DIAG=1`, bounded tracing, and the `[SDIO-ISR]` and `[SDIO-THREAD]` markers.

- [ ] **Step 2: Run the check and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/atbm/check_disconnect_diag.ps1
```

Expected: non-zero exit because the AP diagnostic macros are currently `0` and the two SDIO trace markers do not exist.

### Task 2: Enable AP counters and add bounded SDIO traces

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c`

- [ ] **Step 1: Enable existing AP diagnostics**

Set:

```c
#define ATBM_AP_RX_DIAG 1
#define ATBM_AP_LINK_DIAG 1
```

- [ ] **Step 2: Add bounded controller ISR trace**

For the first 128 IRQs, print the IRQ count, device ID, controller interrupt-enable state, and controller pending state using marker `[SDIO-ISR]`.

- [ ] **Step 3: Add bounded IRQ-thread trace**

For the first 128 processed IRQ messages, print the thread count, CCCR INTx value, ISR count, handler count, and device ID using marker `[SDIO-THREAD]`.

- [ ] **Step 4: Run the check and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/atbm/check_disconnect_diag.ps1
```

Expected: exit code 0 and `ATBM disconnect diagnostics source check passed`.

### Task 3: Rebuild and verify deliverables

**Files:**
- Update: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Update: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Update: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Compile the ATBM WiFi source objects**

Use the existing wireless library build flow and confirm the modified `atbm_gp_sdio_card.c` and ATBM diagnostic sources compile.

- [ ] **Step 2: Archive the fresh WiFi objects**

Recreate `libatbm_wifi.a` from the complete fresh WiFi object set.

- [ ] **Step 3: Link the Debug firmware**

Use the existing project Debug linker inputs and include the math library as required by the project.

- [ ] **Step 4: Verify binary markers**

Confirm these strings exist in both `project.elf` and `project.bin`:

```text
[SDIO-ISR]
[SDIO-THREAD]
[APDIAG] cnt
[WSM-RX-MGMT]
[RX-DISPATCH-MGMT]
```

- [ ] **Step 5: Record hashes and timestamps**

Report fresh SHA256 values and modification times for the archive, ELF, and BIN.

