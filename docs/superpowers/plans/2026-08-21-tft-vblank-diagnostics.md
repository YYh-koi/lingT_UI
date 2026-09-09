# TFT VBlank Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add safe logs that prove whether a queued Bluetooth Cover Art framebuffer reaches the TFT buffer update call.

**Architecture:** The VBlank ISR records only the last consumed frame pointer/base and increments the existing update counter. The existing statistics task prints the raw counter and recorded addresses, keeping UART output out of interrupt context.

**Tech Stack:** C99, CMSIS-RTOS/FreeRTOS, Generalplus display task, PowerShell source-integration test.

---

### Task 1: Lock the Diagnostic Contract

**Files:**
- Modify: `application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1`
- Test: `application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1`

- [ ] **Step 1: Add failing source-contract assertions**

Require the display source to contain `display task: jpeg queued frame=%p base=0x%lx put=%ld` and require the statistics source to contain `TFT_DIAG(update_raw=%lu,last_frame=%p,last_base=0x%lx)`.

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1
```

Expected: failure because the diagnostic strings do not exist yet.

### Task 2: Add ISR-Safe State Sampling

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/src/main.c`
- Modify: `application/task_display/src/ap_display_func.c`

- [ ] **Step 1: Define diagnostic state in task context**

Add global `INT32U tft_last_update_frame` and `INT32U tft_last_update_base` next to the existing TFT counters in `main.c`; declare them `extern` in `ap_display_func.c`.

- [ ] **Step 2: Capture the selected framebuffer in VBlank**

When `frame_new` is non-null, record `(INT32U)frame_new` and `frame_new->base` immediately before `drv_l2_tft_buffer_set()`. Do not print in the ISR.

- [ ] **Step 3: Report raw diagnostics from the statistics task**

Before resetting `fps_tft_update`, print:

```c
DBG_PRINT("TFT_DIAG(update_raw=%lu,last_frame=%p,last_base=0x%lx)\r\n",
    (unsigned long)fps_tft_update, (void *)tft_last_update_frame,
    (unsigned long)tft_last_update_base);
```

- [ ] **Step 4: Report JPEG ready-queue insertion status**

Capture the `ap_display_queue_put()` result for the normal ready-queue path and include `frame`, `base`, `put`, `ifce`, and `ready` in the existing JPEG queued log.

- [ ] **Step 5: Run the focused test and verify GREEN**

Run the Task 1 command. Expected: `PASS: Bluetooth music state and key integration`.

### Task 3: Build and Verify the Diagnostic Firmware

**Files:**
- Generate: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Rebuild the firmware**

Run the existing GPCV2178 Debug make target with the configured ARM toolchain.

- [ ] **Step 2: Verify diagnostic strings and artifact**

Confirm both new log prefixes exist in `project.elf`, generate `project.bin`, and record its timestamp and SHA-256.

