# ATBM SDIO ATM630-Style IRQ Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the Generalplus ATBM SDIO interrupt path to a depth-32 counting semaphore and make the platform IRQ thread the sole IRQ re-enable owner.

**Architecture:** Retain Generalplus `drvl1_sdc_*` controller operations, but copy the ATM630 ISR-to-counting-semaphore-to-worker flow. The worker has a single cleanup exit that releases the host and re-enables the controller IRQ; ATBM RX code only processes data.

**Tech Stack:** C, FreeRTOS counting semaphores, CMSIS task creation, Generalplus SDIO L1/L2, PowerShell structural checks.

---

### Task 1: Add a failing structural regression check

**Files:**
- Create: `tmp/tests/atbm_sdio_atm630_irq_model_check.ps1`

- [x] Assert a depth-32 counting semaphore, ISR-safe give/yield, worker take, common re-enable exit, teardown, and absence of the old CMSIS message queue.
- [x] Assert the ATBM HAL no longer calls `atbm_sdio_host_enable_irq(1)`.
- [x] Run the check and confirm it fails because the current source still uses the one-entry message queue and RX-owned re-enable.

### Task 2: Implement the counting-semaphore interrupt path

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c`

- [x] Replace queue storage and queue diagnostics with semaphore equivalents.
- [x] Make the ISR disable the GP host IRQ, give the counting semaphore with `xSemaphoreGiveFromISR`, and request a context switch when needed.
- [x] Make the worker take the semaphore, read CCCR pending, dispatch the callback, and always release the host and re-enable IRQ from one exit.
- [x] Create the semaphore before the worker and delete it after terminating the worker.

### Task 3: Remove ATBM RX-side IRQ ownership

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/sdio/atbm_sdio.c`

- [x] Remove normal and early-return calls that re-enable the host IRQ.
- [x] Retain RX processing, 100 ms fallback recovery, and diagnostic counters unrelated to re-enable ownership.

### Task 4: Verify

**Files:**
- Verify only.

- [x] Run the focused structural check and existing ATBM private-SDIO checks.
- [x] Build the dedicated ATBM WiFi archive with `tools/wireless_lib/build_wireless_libs.ps1`.
- [x] Inspect the archive/build output and report that runtime hardware validation is still required.
