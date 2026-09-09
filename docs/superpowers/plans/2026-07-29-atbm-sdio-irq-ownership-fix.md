# ATBM SDIO IRQ Ownership Fix Implementation Plan

> Superseded on 2026-08-01 by
> `2026-08-01-atbm-sdio-atm630-irq-model.md`.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the ATBM/Generalplus SDIO IRQ re-enable race that causes `zero pending` and IRQ queue `put_fail` events.

**Architecture:** Treat a matched ATBM SDIO callback as the owner of host IRQ re-enable. Keep platform recovery for unhandled/error events, and make ATBM early-return paths explicitly restore the IRQ.

**Tech Stack:** C, CMSIS-RTOS/FreeRTOS, Generalplus SDIO L1/L2, PowerShell structural regression tests.

---

### Task 1: Add the failing IRQ ownership regression test

**Files:**
- Create: `tmp/tests/atbm_sdio_irq_owner_check.ps1`
- Test: `tmp/tests/atbm_sdio_irq_owner_check.ps1`

- [ ] **Step 1: Write the test**

The test checks that the platform thread tracks whether the callback owns
re-enable, does not unconditionally re-enable after a matched callback, and
that both compiled ATBM IRQ variants restore IRQ on early return.

- [ ] **Step 2: Verify RED**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tmp/tests/atbm_sdio_irq_owner_check.ps1
```

Expected: failure reporting that callback-owned IRQ re-enable is missing.

### Task 2: Implement single-owner IRQ re-enable

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c:72-133`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/sdio/atbm_sdio.c:923-978`
- Test: `tmp/tests/atbm_sdio_irq_owner_check.ps1`

- [ ] **Step 1: Track callback ownership in the platform IRQ thread**

Initialize a per-message `callback_owns_irq_reenable` flag, set it immediately
before calling the matched function callback, and guard the platform re-enable
with `!callback_owns_irq_reenable`.

- [ ] **Step 2: Restore IRQ on ATBM early returns**

Before the `!init_done` and `bh_error` returns in both
`PROCESS_RX_IN_IRQ_THREAD` variants, call
`atbm_sdio_host_enable_irq(1)`.

- [ ] **Step 3: Verify GREEN**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tmp/tests/atbm_sdio_irq_owner_check.ps1
```

Expected: `PASS: ATBM SDIO IRQ re-enable has a single owner.`

### Task 3: Focused regression and build verification

**Files:**
- Verify only; no additional production changes.

- [ ] Run the new ownership test and existing ATBM/private-SDIO structural tests.
- [ ] Build the dedicated ATBM WiFi Debug archive using the repository's wireless library build tooling.
- [ ] Confirm `libatbm_wifi.a` contains the modified private SDIO objects.
- [ ] Generate or refresh the focused source patch if the repository workflow provides one.

Because this workspace is not a Git worktree, test output, archive inspection,
and patch artifacts are the implementation checkpoints instead of commits.
