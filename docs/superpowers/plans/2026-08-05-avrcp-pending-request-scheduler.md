# AVRCP Pending Request Scheduler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve metadata and periodic play-status requests while the existing AVRCP Controller is busy, without adding a general command queue.

**Architecture:** Keep the one-active-command state machine in `avrcp_controller.c`. Add a readable, macro-named pending bitmask and a short retry timer in `btstack_avrcp.c`; coalesce duplicate queries and submit metadata before play status.

**Tech Stack:** C, BTstack AVRCP/AVCTP, BTstack run loop timers, PowerShell source-contract tests, GNU Arm Embedded toolchain.

---

### Task 1: Add a failing scheduler contract test

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_btstack_avrcp_pending_scheduler.ps1`
- Test: `btstack/profiles/btstack_avrcp.c`
- Test: `btstack/at/btstack_at_backend.c`

- [ ] **Step 1: Create the source-contract test**

The test must require:

```powershell
'BTSTACK_AVRCP_PENDING_METADATA'
'BTSTACK_AVRCP_PENDING_PLAY_STATUS'
'BTSTACK_AVRCP_REQUEST_RETRY_MS'
'btstack_avrcp_pending_requests'
'btstack_avrcp_schedule_request'
'btstack_avrcp_try_pending_request'
'btstack_avrcp_clear_pending_requests'
```

It must also assert that metadata is tested before play status, busy results keep
the selected bit pending, successful submission clears only the selected bit,
the progress timer schedules `PLAY_STATUS`, public metadata request schedules
`METADATA`, disconnect clears pending state, and the backend still gates
automatic metadata with `avrcp_config & 1u`.

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
& btstack/ports/gpcv2178/tests/test_btstack_avrcp_pending_scheduler.ps1
```

Expected: FAIL because the pending macros and scheduler helpers do not exist.

### Task 2: Implement the pending bitmask scheduler

**Files:**
- Modify: `btstack/profiles/btstack_avrcp.c`

- [ ] **Step 1: Add readable scheduler definitions and state**

Add:

```c
#define BTSTACK_AVRCP_PENDING_METADATA    (1u << 0)
#define BTSTACK_AVRCP_PENDING_PLAY_STATUS (1u << 1)
#define BTSTACK_AVRCP_REQUEST_RETRY_MS    20u

static uint8_t btstack_avrcp_pending_requests;
static btstack_timer_source_t btstack_avrcp_request_retry_timer;
```

- [ ] **Step 2: Add small scheduler helpers**

Implement:

```c
static void btstack_avrcp_clear_pending_requests(void);
static void btstack_avrcp_schedule_request(uint8_t request_mask);
static void btstack_avrcp_try_pending_request(void);
static void btstack_avrcp_request_retry_timeout(btstack_timer_source_t *timer);
```

`schedule_request` ORs the mask and arms the retry timer instead of submitting
inside an AVRCP callback. `try_pending_request` selects metadata first, calls
the existing Controller API, clears the selected bit only after
`ERROR_CODE_SUCCESS`, retains it after `ERROR_CODE_COMMAND_DISALLOWED`, clears
it for other permanent submission errors, and rearms the timer while any bit
remains.

- [ ] **Step 3: Route existing queries through the scheduler**

Change the progress timeout from:

```c
(void)avrcp_controller_get_play_status(btstack_avrcp_cid);
```

to:

```c
btstack_avrcp_schedule_request(BTSTACK_AVRCP_PENDING_PLAY_STATUS);
```

Change `btstack_avrcp_request_metadata()` to clear the public metadata buffers,
set `BTSTACK_AVRCP_PENDING_METADATA`, and return `BTSTACK_STATUS_OK` once the
connected request is accepted for scheduling.

- [ ] **Step 4: Initialize and clean up scheduler state**

Initialize the pending mask during profile setup. On connection release, remove
the retry timer and clear pending bits before publishing the idle state. Keep
the existing metadata callback and `+TRACKINFO` formatting unchanged.

- [ ] **Step 5: Run focused tests and verify GREEN**

Run:

```powershell
& btstack/ports/gpcv2178/tests/test_btstack_avrcp_pending_scheduler.ps1
& btstack/ports/gpcv2178/tests/test_btstack_gpcv2178_lifecycle.ps1
& btstack/ports/gpcv2178/tests/check_btstack_hci_dump_gpcv2178.ps1
```

Expected: all tests exit 0.

### Task 3: Build and inspect delivery artifacts

**Files:**
- Build: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Rebuild the changed BTstack object and archive**

Remove only the verified generated object
`Project/WirelessLibraries/BTstack_Library/obj/Debug/btstack/profiles/btstack_avrcp.o`,
then run from `Project/WirelessLibraries/BTstack_Library`:

```powershell
$env:Path = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;' + $env:Path
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' `
  -f '../../../output/wireless_libs/build/Xstack_Library_Debug.mk' debug
```

Expected: `btstack_avrcp.c` compiles and `libxstack.a` is refreshed.

- [ ] **Step 2: Relink the main project and regenerate binary**

Run from `Project/GPCV2178_Turnkey_SDK`:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' `
  -f '../../output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk' debug
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-objcopy.exe' `
  -O binary prjDebug/bin/Debug/project.elf prjDebug/bin/Debug/project.bin
```

Expected: both commands exit 0.

- [ ] **Step 3: Inspect the final ELF**

Use `arm-none-eabi-strings.exe` to require `+TRACKINFO` support and the existing
DAC start marker, and reject UART positioning/HCI dump markers. Confirm the ELF,
BIN, and `libxstack.a` timestamps are current.

- [ ] **Step 4: Run final focused verification**

Rerun the scheduler, lifecycle, AT protocol, DAC playback, and HCI dump tests.
Expected: all exit 0. The workspace is not a Git worktree, so commit steps are
not applicable.
