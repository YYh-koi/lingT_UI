# BTstack AT Worker and Ring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep API and AT as mutually exclusive build variants while migrating the AT task, its three FreeRTOS queues, and the 20 ms polling loop to the common worker-ops and fixed-capacity `btstack_ring` architecture.

**Architecture:** Add a dedicated internal `BTSTACK_WORKER_AT` and a completion signal. The common AT frontend owns fixed input/completion/event rings protected by `btstack_platform` critical sections. Producers enqueue without blocking and notify after unlocking; the AT worker blocks in `worker.wait()` until work or completion arrives. The GPCV2178 port owns the static AT task resources. Enabling Bluetooth in an AT build starts only the existing STACK worker in addition to the AT worker; API/EVENT workers remain absent.

**Tech Stack:** C99/C++11 public headers, BTstack 1.6.2, fixed `btstack_ring`, `btstack_platform`/worker ops, FreeRTOS only inside the GPCV2178 port, PowerShell contract/runtime tests, ARM GCC and G+ project builds.

---

## Task 1: Extend the platform worker contract for AT

**Files:**
- Modify: `btstack/include/port/btstack_worker_ops.h`
- Modify: `btstack/include/port/btstack_platform.h`
- Modify: `btstack/core/btstack_platform.c`
- Modify: `btstack/ports/gpcv2178/btstack_rtos_gpcv2178.c`
- Test: `btstack/core/tests/test_btstack_platform.c`
- Test: `btstack/core/tests/test_btstack_platform.ps1`
- Test: `btstack/core/tests/test_btstack_rtos_gpcv2178.c`
- Test: `btstack/core/tests/test_btstack_rtos_gpcv2178.ps1`

- [ ] Add failing tests for `BTSTACK_WORKER_AT`, `BTSTACK_WORKER_SIGNAL_COMPLETION`, the platform time wrapper, AT worker validation/start/notify/wait/stop, and static AT task resources.
- [ ] Run the focused tests and capture the expected RED failures:
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\core\tests\test_btstack_platform.ps1`
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\core\tests\test_btstack_rtos_gpcv2178.ps1`
- [ ] Add `BTSTACK_WORKER_AT` without changing existing worker identities unnecessarily; add `BTSTACK_WORKER_SIGNAL_COMPLETION` as a distinct bit.
- [ ] Add `btstack_platform_time_ms()` that forwards the already-required `platform->system.time_ms` callback and has a safe inactive fallback.
- [ ] Add the GPCV2178 AT worker static stack/TCB/handle entry and explicit validation/defaulting. Keep API/EVENT/STACK behavior unchanged.
- [ ] Verify both focused tests GREEN with `-Wall -Wextra -Werror`, including invalid IDs, repeated start/stop, notification masks, and no direct AT-common RTOS dependency.
- [ ] Checkpoint: record changed files and RED/GREEN output; this workspace has no Git metadata, so no commit is created.

## Task 2: Replace AT input ownership with a fixed ring and AT worker

**Files:**
- Modify: `btstack/at/btstack_at.c`
- Modify if required: `btstack/include/at/btstack_at.h`
- Create: `btstack/at/tests/test_btstack_at_worker.c`
- Create: `btstack/at/tests/test_btstack_at_worker.ps1`
- Modify: `btstack/at/tests/test_btstack_at_task_contract.ps1`

- [ ] Build a host test that links the real `btstack_at.c` and real `btstack_ring.c`, while mocking only platform worker/time and AT backend boundaries.
- [ ] First assert RED: production AT still includes FreeRTOS queue/task headers and cannot compile in the RTOS-free host fixture.
- [ ] Add typed fixed storage and `btstack_ring_t` instances for Input(8), Completion(1), and Event(16); initialize them without heap allocation.
- [ ] Change `btstack_at_start()` to validate/start exactly `BTSTACK_WORKER_AT`; make partial failure retryable and preserve existing public return codes.
- [ ] Change `btstack_at_submit()` to push atomically under the platform critical section, return queue-full without blocking, unlock, then notify `WORK`.
- [ ] Implement the AT worker entry as a blocking signal loop. It must process one command, then drain pending events, and block forever when no input/event exists—no periodic polling.
- [ ] Test input FIFO, capacity/full behavior, callback context, start failure/retry, and that pure AT startup starts AT but not API/EVENT workers.
- [ ] Run:
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\at\tests\test_btstack_at_worker.ps1`
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\at\tests\test_btstack_at_task_contract.ps1`
- [ ] Checkpoint: record file list and RED/GREEN evidence.

## Task 3: Preserve completion timeout and stale-result semantics

**Files:**
- Modify: `btstack/at/btstack_at.c`
- Extend: `btstack/at/tests/test_btstack_at_worker.c`

- [ ] Add failing behavioral cases for synchronous completion, delayed completion, a stale generation/command result, completion-ring full, and exact 1000 ms total timeout.
- [ ] Replace the completion FreeRTOS queue with the one-entry completion ring. `btstack_at_complete()` must push under the platform critical section and notify `COMPLETION` after unlocking.
- [ ] Before dispatch, discard stale completion entries using the same command/generation rules as the current implementation.
- [ ] While a command is pending, wait only for `COMPLETION` using the remaining deadline from `btstack_platform_time_ms()`; leave `WORK` pending for subsequent input/events.
- [ ] Ensure timeout is total wall time, not 1000 ms per wakeup, and output remains serialized on the AT worker.
- [ ] Run the AT worker test repeatedly and confirm no busy-spin, no callback in producer context, and no behavior change in AT result formatting.
- [ ] Checkpoint: record timing assertions and GREEN output.

## Task 4: Migrate AT event delivery and remove the 20 ms poll

**Files:**
- Modify: `btstack/at/btstack_at.c`
- Extend: `btstack/at/tests/test_btstack_at_worker.c`
- Modify: `btstack/at/tests/test_btstack_at_task_contract.ps1`
- Modify if necessary: `btstack/at/tests/test_btstack_at_backend_contract.ps1`

- [ ] Add failing tests proving `btstack_at_post_event()` never calls the output callback directly, event FIFO order is stable, an event arriving during command completion is retained, and a full event ring increments the existing overflow counter.
- [ ] Replace the event FreeRTOS queue with the 16-entry event ring. Push atomically, return queue-full immediately on saturation, then notify `WORK` only after unlocking.
- [ ] Preserve state-event deduplication and output order: command result first, then accumulated asynchronous events.
- [ ] Remove `BTSTACK_AT_EVENT_POLL_MS`, `vTaskDelay`, queue polling, and all FreeRTOS includes/tokens from production `btstack/at`.
- [ ] Strengthen the static contract to require ring/worker/platform APIs and forbid `FreeRTOS.h`, `queue.h`, `task.h`, `xQueue*`, `xTask*`, and the 20 ms polling constant.
- [ ] Run AT parser/protocol/table/backend/worker/task contracts and the common platform boundary contract.
- [ ] Checkpoint: record queue saturation and no-polling evidence.

## Task 5: Integrate the AT-selected build without API/AT coexistence

**Files:**
- Modify if required: `btstack/Makefile.inc`
- Modify if required: `btstack/ports/gpcv2178/Makefile.inc`
- Modify if required: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- Modify generated file only via its generator: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library_Debug.mk`
- Modify: `btstack/README.md`
- Modify: `btstack/ports/README.md`
- Modify relevant layout/build contracts under `btstack/core/tests` and `btstack/api/tests`

- [ ] Add contract assertions that AT selects common sources + public AT frontend + one GPCV2178 port, and API selects common sources + API frontend + one port; neither selected build contains the other frontend/worker ownership.
- [ ] Document pure AT lifecycle: AT worker at frontend start, STACK worker after `AT+BTEN=1`, no API/EVENT workers, three fixed rings, notification-based blocking, and event-overflow behavior.
- [ ] Regenerate selected project makefiles using their official generators; never hand-edit generated source lists.
- [ ] Build:
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1 -Jobs 4`
  - `powershell.exe -NoProfile -ExecutionPolicy Bypass -File Project\WirelessLibraries\BTstack_AT_Library\build_btstack_at.ps1 -Jobs 4`
- [ ] Verify archive members/symbols are unique, AT/common sources are fresh, and no new platform/AT warnings are introduced.
- [ ] Checkpoint: record project counts, archive evidence, and warnings.

## Task 6: Full fresh verification and acceptance

**Files:**
- Test only; production edits are allowed only for defects exposed by fresh verification and must follow a new RED/GREEN cycle.

- [ ] Enumerate every PowerShell test independently under `btstack/api/tests`, `btstack/profiles/tests`, `btstack/core/tests`, `btstack/transport/tests`, and `btstack/at/tests`; run each in a fresh `powershell.exe` process and report exact pass/fail counts.
- [ ] Rebuild API, AT, and legacy archives with their official project generators/builds; force-build the main firmware.
- [ ] Confirm zero direct RTOS queue/task dependencies in common and AT production paths; RTOS task creation/notification remains only in the selected GPCV2178 port.
- [ ] Confirm pure AT runtime worker set is AT before enable and AT+STACK after enable; API worker set remains API+EVENT+STACK.
- [ ] Confirm no heap allocation, ring capacities 8/1/16, existing API queues 8/16/16 and reserve 4 unchanged, and all public AT commands/responses remain source-compatible.
- [ ] Inspect archive/ELF symbols, header hashes, source/object/archive freshness, and warning categories. Clearly separate existing upstream/SDK warnings from new warnings.
- [ ] Perform final specification review, then code-quality review. Resolve every Critical/Important finding with a focused RED/GREEN test and re-review.
- [ ] State target caveats explicitly: host and cross-build verification do not replace board tests for AT UART timing, event bursts, long-running commands, disconnects, or controller behavior.

