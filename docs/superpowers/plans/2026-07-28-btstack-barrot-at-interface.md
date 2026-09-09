# BTstack BARROT AT Interface Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a transport-independent BARROT-compatible AT command task that safely dispatches the approved Classic, HFP HF, A2DP Sink, and basic AVRCP controls into the GPCV2178 BTstack run loop.

**Architecture:** Upper tasks copy complete commands into a static FreeRTOS queue. A dedicated AT task parses commands and posts normalized operations through `btstack_run_loop_execute_on_main_thread()`; BTstack completions and events return to the AT task for strict text formatting and a single-context output callback.

**Tech Stack:** C11-compatible embedded C, FreeRTOS static tasks/queues, BlueKitchen BTstack, GPCV2178 project metadata, ARM GCC, PowerShell verification scripts.

**Repository note:** This source export has no `.git` metadata, so the commit steps normally required by this workflow are replaced by explicit verification checkpoints.

---

### Task 1: Public and Internal Contracts

**Files:**
- Create: `btstack/include/at/btstack_at.h`
- Create: `btstack/at/btstack_at_internal.h`
- Create: `btstack/at/tests/test_btstack_at_parser.c`

- [ ] **Step 1: Write the failing contract test**

Create a test that includes both headers and asserts the approved constants:

```c
#include "at/btstack_at.h"
#include "../btstack_at_internal.h"

_Static_assert(BTSTACK_AT_COMMAND_MAX == 128u, "command bound");
_Static_assert(BTSTACK_AT_INPUT_QUEUE_LEN == 8u, "input depth");
_Static_assert(BTSTACK_AT_EVENT_QUEUE_LEN == 16u, "event depth");

int main(void) {
    btstack_at_command_t command;
    command.id = BTSTACK_AT_COMMAND_AT;
    return command.id == BTSTACK_AT_COMMAND_AT ? 0 : 1;
}
```

- [ ] **Step 2: Compile and verify RED**

Run ARM GCC syntax compilation against the missing headers. Expected result:
failure because `at/btstack_at.h` does not exist.

- [ ] **Step 3: Define the contracts**

The public header defines `btstack_at_output_callback_t`,
`btstack_at_config_t`, submit status codes, and `init/start/submit`.
The internal header defines:

```c
typedef enum {
    BTSTACK_AT_COMMAND_AT,
    BTSTACK_AT_COMMAND_PROFILE,
    BTSTACK_AT_COMMAND_VER,
    BTSTACK_AT_COMMAND_BTEN,
    BTSTACK_AT_COMMAND_ADDR,
    BTSTACK_AT_COMMAND_NAME,
    BTSTACK_AT_COMMAND_PIN,
    BTSTACK_AT_COMMAND_SSP,
    BTSTACK_AT_COMMAND_CFM,
    BTSTACK_AT_COMMAND_PAIR,
    BTSTACK_AT_COMMAND_PAGE,
    BTSTACK_AT_COMMAND_COD,
    BTSTACK_AT_COMMAND_PLIST,
    BTSTACK_AT_COMMAND_TDL,
    BTSTACK_AT_COMMAND_SCAN,
    BTSTACK_AT_COMMAND_DSCA,
    BTSTACK_AT_COMMAND_MICMUTE,
    BTSTACK_AT_COMMAND_SPKVOL,
    BTSTACK_AT_COMMAND_HFPCFG,
    BTSTACK_AT_COMMAND_HFPSTAT,
    BTSTACK_AT_COMMAND_HFPCONN,
    BTSTACK_AT_COMMAND_HFPDISC,
    BTSTACK_AT_COMMAND_HFPDIAL,
    BTSTACK_AT_COMMAND_HFPDTMF,
    BTSTACK_AT_COMMAND_HFPANSW,
    BTSTACK_AT_COMMAND_HFPCHUP,
    BTSTACK_AT_COMMAND_HFPMCAL,
    BTSTACK_AT_COMMAND_HFPADTS,
    BTSTACK_AT_COMMAND_HFPVR,
    BTSTACK_AT_COMMAND_A2DPSTAT,
    BTSTACK_AT_COMMAND_A2DPCONN,
    BTSTACK_AT_COMMAND_A2DPDISC,
    BTSTACK_AT_COMMAND_A2DPMUTE,
    BTSTACK_AT_COMMAND_AVRCPCFG,
    BTSTACK_AT_COMMAND_PLAYPAUSE,
    BTSTACK_AT_COMMAND_PLAY,
    BTSTACK_AT_COMMAND_PAUSE,
    BTSTACK_AT_COMMAND_STOP,
    BTSTACK_AT_COMMAND_NEXT,
    BTSTACK_AT_COMMAND_PREV,
    BTSTACK_AT_COMMAND_FORWARD,
    BTSTACK_AT_COMMAND_BACKWARD,
    BTSTACK_AT_COMMAND_REPEAT,
    BTSTACK_AT_COMMAND_SHUFFLE,
    BTSTACK_AT_COMMAND_SABSUP,
    BTSTACK_AT_COMMAND_SABSDOWN,
    BTSTACK_AT_COMMAND_UNSUPPORTED
} btstack_at_command_id_t;
```

`btstack_at_command_t` carries a form, two bounded text arguments, three
integer arguments, and an optional MAC address. Result and event structures
carry bounded copied data and never point into HCI packets.

- [ ] **Step 4: Compile and verify GREEN**

Run the same contract compilation. Expected result: zero compiler errors.

### Task 2: Strict Parser

**Files:**
- Create: `btstack/at/btstack_at_parser.c`
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: `btstack/at/tests/test_btstack_at_parser.c`

- [ ] **Step 1: Add failing table-driven parser cases**

The table must include every approved command and representative failures:

```c
static const parse_case_t cases[] = {
    { "AT", BTSTACK_AT_COMMAND_AT, BTSTACK_AT_FORM_EXECUTE, 1 },
    { "AT\r\n", BTSTACK_AT_COMMAND_AT, BTSTACK_AT_FORM_EXECUTE, 1 },
    { "AT+PROFILE", BTSTACK_AT_COMMAND_PROFILE, BTSTACK_AT_FORM_QUERY, 1 },
    { "AT+PROFILE=424", BTSTACK_AT_COMMAND_PROFILE, BTSTACK_AT_FORM_SET, 1 },
    { "AT+PROFILE=1192", BTSTACK_AT_COMMAND_PROFILE, BTSTACK_AT_FORM_SET, 0 },
    { "AT+NAME=ABC,1", BTSTACK_AT_COMMAND_NAME, BTSTACK_AT_FORM_SET, 1 },
    { "AT+SCAN=1,10,Phone", BTSTACK_AT_COMMAND_SCAN, BTSTACK_AT_FORM_SET, 1 },
    { "AT+SCAN=2", BTSTACK_AT_COMMAND_SCAN, BTSTACK_AT_FORM_SET, 0 },
    { "AT+HFPCONN=1c5cf226d773", BTSTACK_AT_COMMAND_HFPCONN, BTSTACK_AT_FORM_SET, 1 },
    { "AT+HFPDTMF=#", BTSTACK_AT_COMMAND_HFPDTMF, BTSTACK_AT_FORM_SET, 1 },
    { "AT+AVRCPCFG=3", BTSTACK_AT_COMMAND_AVRCPCFG, BTSTACK_AT_FORM_SET, 1 },
    { "AT+AVRCPCFG=16", BTSTACK_AT_COMMAND_AVRCPCFG, BTSTACK_AT_FORM_SET, 0 },
    { "AT+GETMP", BTSTACK_AT_COMMAND_UNSUPPORTED, BTSTACK_AT_FORM_EXECUTE, 1 },
    { "at+play", BTSTACK_AT_COMMAND_UNSUPPORTED, BTSTACK_AT_FORM_EXECUTE, 0 },
    { " AT", BTSTACK_AT_COMMAND_UNSUPPORTED, BTSTACK_AT_FORM_EXECUTE, 0 },
    { "AT\nAT", BTSTACK_AT_COMMAND_UNSUPPORTED, BTSTACK_AT_FORM_EXECUTE, 0 }
};
```

Add boundary cases for names, PIN, phone number, decimal ranges, MAC length,
optional parameters, and trailing separators.

- [ ] **Step 2: Build and run parser test to verify RED**

Compile the parser test with `btstack_at_parser.c`. Expected result: link
failure for missing `btstack_at_parse()`.

- [ ] **Step 3: Implement minimal strict parser**

Implement:

```c
int btstack_at_parse(
    const char *input,
    uint16_t length,
    btstack_at_command_t *command);
```

Normalize only a final CRLF, reject every other CR/LF/NUL, split the command
name from `=`, identify the form from a table, and pass the parameter substring
to a command-specific validator. Convert MAC input to six bytes and preserve
bounded text fields without heap allocation.

- [ ] **Step 4: Run parser test to verify GREEN**

Expected result: all table rows pass and the process exits with status zero.

### Task 3: BARROT Formatter

**Files:**
- Create: `btstack/at/btstack_at_protocol.c`
- Create: `btstack/at/tests/test_btstack_at_protocol.c`
- Modify: `btstack/at/btstack_at_internal.h`

- [ ] **Step 1: Write failing golden-byte tests**

Test exact byte arrays for:

```c
"OK\r\n"
"ERROR\r\n"
"+ADDR=1C5CF226D773\r\nOK\r\n"
"+PLIST=1\x0C424\x0C1C5CF226D773\x0CPhone\r\n"
"+PLIST=E\r\n"
"+HFPVR=1\r\n"
"+TRACKINFO=Title\x0CArtist\x0CAlbum\r\n"
```

Test output-buffer overflow and verify it returns an error without partial
protocol text.

- [ ] **Step 2: Build and run formatter test to verify RED**

Expected result: missing formatter symbols.

- [ ] **Step 3: Implement bounded formatting**

Implement result and event formatters using `snprintf` plus explicit byte
append for `0x0C`. Provide a helper that formats one complete response or
event into a caller-owned 512-byte buffer.

- [ ] **Step 4: Run formatter test to verify GREEN**

Expected result: every golden byte array matches in length and content.

### Task 4: Dedicated AT Task and Static Queues

**Files:**
- Create: `btstack/at/btstack_at.c`
- Create: `btstack/at/tests/test_btstack_at_task_contract.ps1`
- Modify: `btstack/at/btstack_at_internal.h`

- [ ] **Step 1: Write failing structural checks**

The PowerShell test requires:

```powershell
$source -match 'xQueueCreateStatic'
$source -match 'xTaskCreateStatic'
$source -match 'btstack_run_loop_execute_on_main_thread'
$source -match 'xQueueReceive'
$source -notmatch 'pvPortMalloc|malloc\s*\('
```

It also verifies that `btstack_at_submit()` uses a zero-wait queue send and
that only the AT task calls the configured output callback.

- [ ] **Step 2: Run structural test to verify RED**

Expected result: failure because `btstack_at.c` does not exist.

- [ ] **Step 3: Implement the task**

Create static input, completion, and event queues. `btstack_at_submit()` copies
the exact supplied length into a queue item. The AT task parses one item,
dispatches one normalized operation through a registered backend function,
waits up to `BTSTACK_AT_EXEC_TIMEOUT_MS`, emits the command result, and then
drains queued events.

Expose internal backend hooks:

```c
int btstack_at_backend_register(
    btstack_at_backend_dispatch_t dispatch);
int btstack_at_complete(
    const btstack_at_result_t *result);
int btstack_at_post_event(
    const btstack_at_event_t *event);
```

These hooks copy messages and never block the BTstack task.

- [ ] **Step 4: Run structural and ARM syntax checks to verify GREEN**

Expected result: all structural checks pass and ARM GCC emits no diagnostics.

### Task 5: GPCV2178 General and GAP Backend

**Files:**
- Create: `btstack/ports/gpcv2178/btstack_at_gpcv2178.h`
- Create: `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
- Create: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/ports/gpcv2178/btstack_config.h`

- [ ] **Step 1: Write failing backend contract checks**

Require a persistent main-thread callback registration, call to
`btstack_run_loop_execute_on_main_thread()`, state snapshot, eight RAM peers,
and switch cases for every General/Classic command ID.

- [ ] **Step 2: Run backend contract check to verify RED**

Expected result: missing backend files and cases.

- [ ] **Step 3: Implement general operations**

Register the backend after run-loop initialization. Implement power, address,
name, PIN, SSP, confirmation, discoverability, connectability, class of
device, RAM peer list, Classic inquiry, disconnect-all, microphone mute, and
speaker volume. Use BTstack GAP/HCI APIs only inside the run-loop callback.

Track inquiry results, pairing requests/results, the active ACL handle, and
unexpected disconnect reason. Post normalized events without formatting them.

- [ ] **Step 4: Run backend checks and ARM syntax compilation**

Expected result: structural checks pass and the backend compiles with the
GPCV2178 include set.

### Task 6: HFP HF Operations and Events

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`

- [ ] **Step 1: Extend failing checks for every HFP command and event**

Require all HFP command IDs, service-level connect/disconnect, dial/redial,
DTMF, answer/hangup, call waiting controls, SCO transfer, voice recognition,
and mappings for every approved `+HFP...` event.

- [ ] **Step 2: Run contract check to verify RED**

Expected result: missing HFP cases and forwarding calls.

- [ ] **Step 3: Implement HFP mapping**

Use `hfp_hf_*` APIs inside the backend dispatcher. Convert HFP subevents into
the 0 through 10 BARROT state model, cache call numbers and AG indicators, and
distinguish requested disconnect from link loss.

- [ ] **Step 4: Run HFP checks and focused classic-profile verification**

Run the backend contract and existing
`tmp/tests/check_btstack_classic_profiles.ps1`. Expected result: both pass.

### Task 7: A2DP Sink and AVRCP Operations and Events

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`

- [ ] **Step 1: Extend failing checks for A2DP/AVRCP**

Require all approved command IDs, A2DP connect/disconnect, mute state, AVRCP
playback/navigation/repeat/shuffle/volume calls, connection state, playback
state, progress, metadata, and absolute-volume event mapping.

- [ ] **Step 2: Run contract check to verify RED**

Expected result: missing A2DP/AVRCP cases and event forwarding.

- [ ] **Step 3: Implement A2DP/AVRCP mapping**

Store the A2DP and AVRCP CIDs, active remote address, cached playback state,
metadata fields, and configured progress interval. Use a BTstack run-loop
timer for progress queries. Continue discarding media payloads.

- [ ] **Step 4: Run backend and classic-profile checks**

Expected result: contract and existing profile checks pass.

### Task 8: Port and Project Integration

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Create: `tmp/tests/check_btstack_at_interface.ps1`

- [ ] **Step 1: Write failing integration check**

Require the public header, all AT sources, backend registration after run-loop
initialization, project units for each C source, and include paths for
`btstack/include` and `btstack/at`.

- [ ] **Step 2: Run integration check to verify RED**

Expected result: missing project units and registration call.

- [ ] **Step 3: Integrate sources**

Register the backend before profile setup and controller power-on. Add all
sources to the make fragment and GPC project without changing unrelated unit
options.

- [ ] **Step 4: Run integration and existing boundary checks**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tmp/tests/check_btstack_at_interface.ps1
powershell -ExecutionPolicy Bypass -File tmp/tests/check_btstack_classic_profiles.ps1
powershell -ExecutionPolicy Bypass -File btstack/tools/check_adapter_boundary.ps1
```

Expected result: every command exits zero.

### Task 9: Final Verification

**Files:**
- Modify only if a verification failure requires a tested correction.

- [ ] **Step 1: Run parser and formatter executable tests**

Expected result: every test case passes with zero failures.

- [ ] **Step 2: Run all AT structural and integration checks**

Expected result: zero failed checks.

- [ ] **Step 3: Compile every new C source with ARM GCC**

Use the GPCV2178 CPU and include configuration. Expected result: no compiler
errors or warnings introduced by the new sources.

- [ ] **Step 4: Run focused existing BTstack regression checks**

Run the classic-profile, H5 logging, and crash-boundary scripts. Expected
result: all exit zero.

- [ ] **Step 5: Reconcile implementation against the approved design**

Verify every whitelist command has a parser row and backend case, every
unsupported group returns plain `ERROR`, all BTstack calls remain in the
run-loop task, and all user output remains in the AT task.
