# xStack Independent API Copy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create an independent `btstack_api/` source copy and `libxstack_api.a` delivery whose application surface is typed `bt_xstack_*` C APIs instead of AT text commands, without changing the existing `btstack/` implementation.

**Architecture:** Copy the complete stack, keep transport/core/GAP/Profile code unchanged, and replace the compiled AT front end with a static request queue, API worker, run-loop dispatcher, synchronous state snapshot getters, and domain callbacks. The worker serializes fire-and-forget requests; an internal completion handshake protects the run-loop callback registration but is never exposed to applications.

**Tech Stack:** C99, FreeRTOS static queues/tasks, BTstack run loop, PowerShell contract tests, Generalplus G+ project/make export, ARM GCC 4.9.3.

---

## File Map

**Create:**

- `btstack_api/` — mechanical source copy of `btstack/`.
- `btstack_api/include/api/bt_xstack.h` — umbrella public header.
- `btstack_api/include/api/bt_xstack_types.h` — public common types and result codes.
- `btstack_api/include/api/bt_xstack_service.h` — lifecycle, GAP, pairing, scan, and connection API.
- `btstack_api/include/api/bt_xstack_phone.h` — HFP API and phone events.
- `btstack_api/include/api/bt_xstack_audio.h` — A2DP, AVRCP, PCM, and media events.
- `btstack_api/api/bt_xstack_api_internal.h` — internal requests, state, completion, and event envelopes.
- `btstack_api/api/bt_xstack_api_core.c` — public wrappers, static queues, worker, and getters.
- `btstack_api/api/bt_xstack_api_backend.c` — run-loop request execution and state ownership.
- `btstack_api/api/bt_xstack_api_event.c` — domain callback delivery from the worker.
- `btstack_api/api/tests/test_bt_xstack_api_headers.c` — standalone public-header compile contract.
- `btstack_api/api/tests/test_bt_xstack_api_contract.ps1` — API coverage and no-AT dependency contract.
- `btstack_api/api/tests/test_bt_xstack_api_queue_contract.ps1` — static queue/threading contract.
- `tests/test_xstack_api_copy_contract.ps1` — original/copy isolation contract.
- `Project/WirelessLibraries/Xstack_API_Library/` — isolated library project.
- `output/wireless_libs/build/Xstack_API_Library_Debug.mk` — command-line verification makefile.
- `gplib/liba_wifi_src/atbm/atbm6165/bt_api/` — public delivery directory.

**Modify only inside the copy:**

- `btstack_api/adapter/bt_xstack_manager.c` — remove AT lifecycle and delegate to the API core.
- `btstack_api/adapter/Makefile.inc` — replace AT sources with API sources.
- `btstack_api/ports/gpcv2178/btstack_gpcv2178_port.c` — initialize the API backend instead of the AT backend.

The original `btstack/` and `Project/WirelessLibraries/BTstack_Library/` remain byte-for-byte unchanged.

### Task 1: Establish the copy isolation contract

**Files:**
- Create: `tests/test_xstack_api_copy_contract.ps1`

- [ ] **Step 1: Write the failing isolation test**

The test records the original stack manifest and requires an independent copy without path references back to `btstack/`:

```powershell
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'btstack'
$copy = Join-Path $root 'btstack_api'
if (-not (Test-Path $copy)) { throw 'Missing btstack_api source copy' }
$required = @('3rd-party','adapter','events','gap','ports','profiles','src','transport')
foreach ($name in $required) {
    if (-not (Test-Path (Join-Path $copy $name))) {
        throw "Missing copied stack directory: $name"
    }
}
if (-not (Test-Path (Join-Path $copy 'api'))) { throw 'Missing API front end' }
if (Test-Path (Join-Path $copy 'include\adapter\bt_xstack.h')) {
    $legacy = Get-Content -Raw (Join-Path $copy 'include\adapter\bt_xstack.h')
    if ($legacy -match 'bt_xstack_send_at') { throw 'API copy still exports the AT facade' }
}
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/test_xstack_api_copy_contract.ps1
```

Expected: failure containing `Missing btstack_api source copy`.

- [ ] **Step 3: Copy the stack mechanically**

Use a validated explicit source and destination:

```powershell
$source = (Resolve-Path 'btstack').Path
$destination = Join-Path (Resolve-Path '.').Path 'btstack_api'
if (Test-Path $destination) { throw "Destination already exists: $destination" }
Copy-Item -LiteralPath $source -Destination $destination -Recurse
New-Item -ItemType Directory -Path (Join-Path $destination 'api') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $destination 'include\api') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $destination 'api\tests') | Out-Null
```

- [ ] **Step 4: Re-run the isolation test**

Expected: it advances past the missing-copy checks and fails only because the new API facade has not yet replaced the copied public header.

- [ ] **Step 5: Record a no-VCS checkpoint**

Because the SDK directory has no Git metadata and SVN is unavailable, record hashes instead of committing:

```powershell
Get-ChildItem btstack -Recurse -File | Get-FileHash -Algorithm SHA256 |
  Sort-Object Path | Export-Clixml tmp/xstack_api_original_manifest.xml
```

### Task 2: Define the public API headers

**Files:**
- Create: `btstack_api/include/api/bt_xstack_types.h`
- Create: `btstack_api/include/api/bt_xstack_service.h`
- Create: `btstack_api/include/api/bt_xstack_phone.h`
- Create: `btstack_api/include/api/bt_xstack_audio.h`
- Create: `btstack_api/include/api/bt_xstack.h`
- Create: `btstack_api/api/tests/test_bt_xstack_api_headers.c`
- Create: `btstack_api/api/tests/test_bt_xstack_api_contract.ps1`

- [ ] **Step 1: Write the failing public-header compile contract**

```c
#include "bt_xstack.h"

static void service_event(const bt_xstack_service_event_t *event, void *context) {
    (void)event; (void)context;
}

int main(void) {
    bt_xstack_config_t config = {0};
    bt_xstack_state_t state;
    config.service_callback = service_event;
    if (bt_xstack_init(&config) != BT_XSTACK_OK) return 1;
    if (bt_xstack_get_state(&state) != BT_XSTACK_OK) return 2;
    return 0;
}
```

The PowerShell contract must enumerate every current `BTSTACK_AT_COMMAND_*` entry and its corresponding `bt_xstack_*` declaration, then reject `bt_xstack_send_at`, `btstack_at_`, and `#include "at/` in all five public headers.

- [ ] **Step 2: Run the contracts and verify failure**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack_api/api/tests/test_bt_xstack_api_contract.ps1
```

Expected: failure for missing `bt_xstack_types.h` and API declarations.

- [ ] **Step 3: Implement `bt_xstack_types.h`**

Define fixed-width, pointer-free public values:

```c
typedef enum {
    BT_XSTACK_OK = 0,
    BT_XSTACK_INVALID_ARGUMENT = -1,
    BT_XSTACK_NOT_INITIALIZED = -2,
    BT_XSTACK_CONFIGURATION_CONFLICT = -3,
    BT_XSTACK_QUEUE_FULL = -4,
    BT_XSTACK_NOT_READY = -5,
    BT_XSTACK_INTERNAL_ERROR = -6
} bt_xstack_result_t;

typedef struct { uint8_t bytes[6]; } bt_xstack_addr_t;

typedef enum {
    BT_XSTACK_PROFILE_HFP_HF = 0x01,
    BT_XSTACK_PROFILE_A2DP_SINK = 0x04,
    BT_XSTACK_PROFILE_AVRCP = 0x80
} bt_xstack_profile_t;
```

Also define power, connection, HFP, A2DP, AVRCP, scan, pairing, peer, track, and PCM configuration structures with owned fixed-size arrays.

- [ ] **Step 4: Implement the service, phone, and audio declarations**

Service declarations include lifecycle, power, name/address/PIN/SSP/COD/profile/TDL getters and setters, scan, pairing confirmation, paired-list access, discoverable/connectable controls, disconnect-all, and per-profile connect/disconnect/state.

Phone declarations include HFP config/state getters, last/specified connect, disconnect, dial/redial/DTMF, answer/hang-up, multiparty control, audio toggle/set, voice recognition, microphone mute, and speaker volume.

Audio declarations include A2DP state/connect/disconnect/mute, AVRCP config/state, play/pause/stop/navigation, repeat/shuffle, volume, metadata event structures, and PCM callback.

- [ ] **Step 5: Implement the umbrella header**

```c
#ifndef BT_XSTACK_PUBLIC_H
#define BT_XSTACK_PUBLIC_H
#include "bt_xstack_types.h"
#include "bt_xstack_service.h"
#include "bt_xstack_phone.h"
#include "bt_xstack_audio.h"
#endif
```

- [ ] **Step 6: Re-run header contracts**

Expected: PASS for symbol coverage, standalone include ordering, C compatibility, and absence of AT public symbols.

### Task 3: Implement static request transport and lifecycle

**Files:**
- Create: `btstack_api/api/bt_xstack_api_internal.h`
- Create: `btstack_api/api/bt_xstack_api_core.c`
- Create: `btstack_api/api/tests/test_bt_xstack_api_queue_contract.ps1`
- Modify: `btstack_api/adapter/bt_xstack_manager.c`

- [ ] **Step 1: Write the failing queue/thread contract**

Require these constants and primitives:

```powershell
Assert-Match $internal 'BT_XSTACK_REQUEST_QUEUE_LEN\s+8u'
Assert-Match $internal 'BT_XSTACK_EVENT_QUEUE_LEN\s+16u'
Assert-Match $core 'xQueueCreateStatic'
Assert-Match $core 'xTaskCreateStatic'
Assert-Match $core 'btstack_run_loop_execute_on_main_thread'
Assert-NoMatch $core '\bmalloc\s*\('
Assert-NoMatch $core '\bfree\s*\('
```

It must also require copying strings and addresses into request storage before `xQueueSend` and reject `FromISR` APIs.

- [ ] **Step 2: Run the queue contract and verify failure**

Expected: failure for missing API core.

- [ ] **Step 3: Define the internal request model**

```c
typedef enum {
    BT_XSTACK_REQUEST_SET_POWER,
    BT_XSTACK_REQUEST_SET_PROFILE,
    BT_XSTACK_REQUEST_SET_NAME,
    BT_XSTACK_REQUEST_SET_PIN,
    BT_XSTACK_REQUEST_SET_SSP,
    BT_XSTACK_REQUEST_PAIR_CONFIRM,
    BT_XSTACK_REQUEST_SET_DISCOVERABLE,
    BT_XSTACK_REQUEST_SET_CONNECTABLE,
    BT_XSTACK_REQUEST_SET_COD,
    BT_XSTACK_REQUEST_CLEAR_PEER,
    BT_XSTACK_REQUEST_CLEAR_PEERS,
    BT_XSTACK_REQUEST_SET_TDL,
    BT_XSTACK_REQUEST_SCAN_START,
    BT_XSTACK_REQUEST_SCAN_STOP,
    BT_XSTACK_REQUEST_DISCONNECT_ALL,
    BT_XSTACK_REQUEST_HFP_CONNECT,
    BT_XSTACK_REQUEST_HFP_DISCONNECT,
    BT_XSTACK_REQUEST_HFP_CONTROL,
    BT_XSTACK_REQUEST_HFP_DIAL,
    BT_XSTACK_REQUEST_HFP_DTMF,
    BT_XSTACK_REQUEST_A2DP_CONNECT,
    BT_XSTACK_REQUEST_A2DP_DISCONNECT,
    BT_XSTACK_REQUEST_A2DP_MUTE,
    BT_XSTACK_REQUEST_AVRCP_CONTROL,
    BT_XSTACK_REQUEST_AVRCP_CONFIG,
    BT_XSTACK_REQUEST_AVRCP_REPEAT,
    BT_XSTACK_REQUEST_AVRCP_SHUFFLE
} bt_xstack_request_id_t;
```

The request contains owned `address[6]`, `text[64]`, three numeric values, value count, flags, and a monotonically increasing internal generation.

- [ ] **Step 4: Implement initialization and worker resources**

Use static storage for an 8-item request queue, one-item completion queue, 16-item event queue, task control block, and 2048-word stack. `bt_xstack_init()` validates and copies callbacks, initializes `btstack_gpcv2178`, creates resources, starts the worker, and remains idempotent for byte-identical callback configuration.

- [ ] **Step 5: Implement fire-and-forget submission**

All setters call one helper that validates initialization, copies the full request, performs zero-timeout `xQueueSend`, and returns only `BT_XSTACK_OK` or an immediate validation/queue status.

- [ ] **Step 6: Re-run the queue and isolation contracts**

Expected: PASS for static allocation, request copying, no ISR path, no heap use, and no AT public dependency.

### Task 4: Port the general/GAP backend and synchronous snapshot

**Files:**
- Create: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_contract.ps1`

- [ ] **Step 1: Extend the failing contract for every general command**

The test requires the exact public coverage for `AT`, `PROFILE`, `VER`, `BTEN`, `ADDR`, `NAME`, `PIN`, `SSP`, `CFM`, `PAIR`, `PAGE`, `COD`, `PLIST`, `TDL`, `SCAN`, `DSCA`, `MICMUTE`, and `SPKVOL`.

- [ ] **Step 2: Run it and verify missing implementations fail**

Expected: declarations exist but implementation checks fail.

- [ ] **Step 3: Port general command execution**

Copy the validated behavior from `btstack/at/btstack_at_backend.c` into a request switch that calls the copied GAP/Profile functions only from the run-loop callback. Preserve the current defaults: profile mask `424`, speaker volume `14`, HFP config `3`, AVRCP config `3`, PIN `0000`, SSP `2`, COD `0x240404`, and eight RAM peers.

- [ ] **Step 4: Implement the state snapshot**

The backend owns one `bt_xstack_state_snapshot_t`. Event handlers update it in the run loop. Getters enter the port critical section, copy scalar/fixed-array data into caller-owned buffers, exit the critical section, and return without calling callbacks while locked.

- [ ] **Step 5: Run general API contracts**

Expected: PASS for all general API declarations, dispatch cases, state fields, defaults, and synchronous getters.

### Task 5: Port HFP commands and phone events

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Create: `btstack_api/api/bt_xstack_api_event.c`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_contract.ps1`

- [ ] **Step 1: Add failing HFP coverage checks**

Require API and backend cases for `HFPCFG`, `HFPSTAT`, `HFPCONN`, `HFPDISC`, `HFPDIAL`, `HFPDTMF`, `HFPANSW`, `HFPCHUP`, `HFPMCAL`, `HFPADTS`, and `HFPVR`.

- [ ] **Step 2: Run and verify failure**

Expected: failure for missing HFP backend cases/event mapping.

- [ ] **Step 3: Port HFP execution**

Map requests to `btstack_hfp_hf_connect`, `disconnect`, `dial_number`, `redial`, `send_dtmf`, gain setters, and typed controls. Preserve the last-peer fallback and current state guards.

- [ ] **Step 4: Implement phone event envelopes**

Map every current HFP AT event—state, device, audio, voice recognition, signal, network, roam, battery, number, in-band ring, ring, lost, and multiparty—to an owned fixed-size `bt_xstack_phone_event_t` queued from the run loop.

- [ ] **Step 5: Deliver callbacks from the worker**

The worker drains ordinary events between requests and invokes `phone_callback(event, context)`. It never invokes application code from the run-loop callback.

- [ ] **Step 6: Run HFP contracts**

Expected: PASS for full command/event coverage and worker-thread callback delivery.

### Task 6: Port A2DP, AVRCP, PCM, and audio events

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_event.c`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Test: `btstack_api/api/tests/test_bt_xstack_api_contract.ps1`

- [ ] **Step 1: Add failing audio coverage checks**

Require API/backend cases for `A2DPSTAT`, `A2DPCONN`, `A2DPDISC`, `A2DPMUTE`, `AVRCPCFG`, `PLAYPAUSE`, `PLAY`, `PAUSE`, `STOP`, `NEXT`, `PREV`, `FORWARD`, `BACKWARD`, `REPEAT`, `SHUFFLE`, `SABSUP`, and `SABSDOWN`.

- [ ] **Step 2: Run and verify failure**

Expected: missing audio cases and events.

- [ ] **Step 3: Port A2DP/AVRCP execution**

Reuse the existing A2DP/AVRCP state guards, last-peer fallback, play/pause decision, repeat/shuffle mapping, and pass-through controls.

- [ ] **Step 4: Port audio events**

Map A2DP state/device/lost/PCM config and AVRCP state/play status/play mode/track status/track info/absolute volume into fixed-size `bt_xstack_audio_event_t` objects queued to the worker.

- [ ] **Step 5: Preserve the direct PCM path**

Register one copied sample bridge with `btstack_a2dp_sink_set_audio_sample_callback`. Validate PCM S16, non-null data, and non-zero length, snapshot the configured callback under the critical section, then invoke it outside the critical section without passing through the ordinary event queue.

- [ ] **Step 6: Run audio contracts**

Expected: PASS for full audio command/event coverage and direct PCM behavior.

### Task 7: Replace copied manager/port integration and build the independent library

**Files:**
- Modify: `btstack_api/adapter/bt_xstack_manager.c`
- Modify: `btstack_api/adapter/Makefile.inc`
- Modify: `btstack_api/ports/gpcv2178/btstack_gpcv2178_port.c`
- Create: `Project/WirelessLibraries/Xstack_API_Library/Xstack_API_Library.gpcmprj`
- Create: `output/wireless_libs/build/Xstack_API_Library_Debug.mk`
- Create: `btstack_api/api/tests/test_bt_xstack_api_build_integration.ps1`

- [ ] **Step 1: Write the failing build integration test**

Require:

```powershell
Assert-Match $project 'btstack_api\\api\\bt_xstack_api_core\.c'
Assert-Match $project 'btstack_api\\api\\bt_xstack_api_backend\.c'
Assert-Match $project 'btstack_api\\api\\bt_xstack_api_event\.c'
Assert-NoMatch $project 'btstack_api\\at\\'
Assert-NoMatch $project '\\btstack\\'
Assert-Match $project 'OutputName="xstack_api"'
```

- [ ] **Step 2: Run and verify failure**

Expected: missing API library project.

- [ ] **Step 3: Clone the existing library project mechanically**

Copy `BTstack_Library.gpcmprj`, replace project/output names, replace every `btstack\` source/include root with `btstack_api\`, remove AT source entries, and add the three API sources plus `btstack_api/include/api`.

- [ ] **Step 4: Update copied manager and port integration**

Remove includes and calls to `btstack_at_*` from copied manager/port files. Initialize the API backend and event bridge after adapter/profile setup, while preserving current run-loop start and power-state lifecycle.

- [ ] **Step 5: Produce the make verification file**

Clone the current xStack make export, change object/source roots to `btstack_api`, remove AT objects, add API objects, and set the archive output to `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`.

- [ ] **Step 6: Run build integration contracts**

Expected: PASS with no original-stack references and no API project AT objects.

### Task 8: Package headers and verify the result

**Files:**
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack_types.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack_service.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack_phone.h`
- Create: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack_audio.h`
- Create by build: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`

- [ ] **Step 1: Copy the public headers mechanically**

Copy the five verified public headers from `btstack_api/include/api` into the delivery directory and compare SHA-256 hashes.

- [ ] **Step 2: Run all focused API tests**

```powershell
$tests = @(
  'tests/test_xstack_api_copy_contract.ps1',
  'btstack_api/api/tests/test_bt_xstack_api_contract.ps1',
  'btstack_api/api/tests/test_bt_xstack_api_queue_contract.ps1',
  'btstack_api/api/tests/test_bt_xstack_api_build_integration.ps1'
)
foreach ($test in $tests) {
  powershell -NoProfile -ExecutionPolicy Bypass -File $test
  if ($LASTEXITCODE -ne 0) { throw "Failed: $test" }
}
```

Expected: all tests print PASS and exit 0.

- [ ] **Step 3: Build from a clean API object directory**

Run the available Generalplus make export for `Xstack_API_Library_Debug.mk`. Expected: creation of `libxstack_api.a` without compiling any `btstack_api/at/*.c` file.

- [ ] **Step 4: Inspect archive exports**

Use the SDK ARM `nm` tool and require `bt_xstack_init`, `bt_xstack_enable`, all public command wrappers, getters, and callback registration paths; reject `bt_xstack_send_at`, `btstack_at_parse`, and `btstack_at_format_event`.

- [ ] **Step 5: Run original-stack regressions**

Run the existing manager, AT parser/protocol/backend/task, event router, GAP, Profile, PCM, and GPCV2178 lifecycle contract tests from `btstack/`. Expected: all remain PASS.

- [ ] **Step 6: Verify original tree integrity**

Recalculate the original `btstack/` SHA-256 manifest and compare it to `tmp/xstack_api_original_manifest.xml`. Expected: no hash additions, removals, or changes.

- [ ] **Step 7: Report hardware-only residual validation**

Document that UART/H5 controller communication, pairing, HFP calls, A2DP streaming, AVRCP commands, and PCM playback still require flashing the target board; do not claim board verification from host contracts alone.

## Plan Self-Review

- Spec coverage: source isolation, full current AT capability coverage, typed domain callbacks, asynchronous controls, synchronous getters, no completion event, static memory, independent build, delivery, and regressions are each assigned to a task.
- Placeholder scan: no deferred implementation markers are present.
- Type consistency: all public symbols use `bt_xstack_*`; internal symbols use `bt_xstack_api_*`; the output remains `libxstack_api.a` under `bt_api/`.
- Scope: BLE, PBAP, SPP, RFCOMM, AVRCP browsing/cover art, and AAC remain explicitly excluded.
