# Bluetooth Music Cover Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a MODE-selectable Bluetooth music page that receives AVRCP linked thumbnails, decodes them outside Stack Run Loop context, and displays them complete and centered on CN1.

**Architecture:** `bt_music_art.c` owns bounded Cover Art reception, display-frame ownership, generation filtering, the Bluetooth music state loop, and deferred JPEG decode. `bt_xstack_demo.c` only registers the callback and selects linked-thumbnail mode; State Handling Task decodes and Display Task draws. Existing display buffers are reused as compressed-image storage, so no new large static image buffer or task is introduced.

**Tech Stack:** C99, FreeRTOS application queues, existing xStack public Cover Art/AVRCP API, Generalplus JPEG/scaler/display drivers, PowerShell host tests, ARM GCC, G+ project generator, MinGW make.

---

## File Structure

**Create**

- `application/task_state_handling/state_bt_music/inc/bt_music_art.h` — application-private state and Cover Art callback declarations.
- `application/task_state_handling/state_bt_music/src/bt_music_art.c` — receive state machine, frame ownership, page loop, fit calculation, decode, and draw submission.
- `application/task_state_handling/state_bt_music/tests/test_bt_music_art.c` — host behavior test using real `bt_music_art.c`.
- `application/task_state_handling/state_bt_music/tests/test_bt_music_art.ps1` — C99/Werror test build and execution.
- `application/task_state_handling/state_bt_music/tests/stubs/application.h` — minimal application/display/JPEG test boundary.
- `application/task_state_handling/state_bt_music/tests/stubs/bt_xstack.h` — minimal Cover Art/AVRCP test boundary.
- `application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1` — source/build/state/key contract.

**Modify**

- `application/inc/turnkey_sys_msg.h` — add `STATE_BT_MUSIC`.
- `application/inc/turnkey_drv_msg.h` — add `MSG_APQ_BT_COVER_READY` beside other `MSG_APQ_*` messages.
- `application/task_state_handling/src/task_state_handling.c` — dispatch `STATE_BT_MUSIC`.
- `application/task_state_handling/state_browse/src/state_browse.c` — MODE transitions to Bluetooth music.
- `application/task_peripheral_handling/inc/ap_peripheral_handling.h` — remove the obsolete application-global media-key registration type/API.
- `application/task_peripheral_handling/src/ap_peripheral_handling.c` — stop consuming MODE/NEXT/MENU as global AVRCP controls; leave call-key precedence intact.
- `Project/wifi_demo/bt_xstack_demo.c` — register Cover Art callback, select linked-thumbnail mode, and remove obsolete media-key handler registration.
- `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj` — add `bt_music_art.c` and its include directory exactly once.
- `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk` — regenerate from the project file; do not hand-maintain the generated rule.

The application already has backup source files. Do not edit `*_before_update_backup.c`; only production sources and their focused tests change.

### Task 1: Add the bounded Cover Art receiver

**Files:**
- Create: `application/task_state_handling/state_bt_music/inc/bt_music_art.h`
- Create: `application/task_state_handling/state_bt_music/src/bt_music_art.c`
- Create: `application/task_state_handling/state_bt_music/tests/test_bt_music_art.c`
- Create: `application/task_state_handling/state_bt_music/tests/test_bt_music_art.ps1`
- Create: `application/task_state_handling/state_bt_music/tests/stubs/application.h`
- Create: `application/task_state_handling/state_bt_music/tests/stubs/bt_xstack.h`

- [ ] **Step 1: Write the receiver behavior test first**

The test must drive the real callback with public event shapes and a fixed mock display-frame pool. Cover at least these exact cases:

```c
static void send_started(uint32_t generation);
static void send_data(uint32_t generation, uint32_t offset,
    const uint8_t *data, uint16_t length);
static void send_completed(uint32_t generation, int32_t status,
    uint32_t total_length);

static void test_complete_thumbnail_becomes_latest(void)
{
    static const uint8_t jpeg[] = { 0xff, 0xd8, 0xff, 0xd9 };

    send_started(7u);
    send_data(7u, 0u, jpeg, sizeof(jpeg));
    send_completed(7u, BT_XSTACK_OK, sizeof(jpeg));

    assert(test_ready_generation == 7u);
    assert(test_ready_message_count == 1u);
    assert(memcmp(test_latest_bytes(), jpeg, sizeof(jpeg)) == 0);
}

static void test_rejects_oversize_and_out_of_order(void)
{
    uint8_t byte = 0x5au;

    send_started(8u);
    send_data(8u, BT_MUSIC_ART_MAX_JPEG_SIZE, &byte, 1u);
    send_completed(8u, BT_XSTACK_OK,
        BT_MUSIC_ART_MAX_JPEG_SIZE + 1u);
    assert(test_ready_message_count == 0u);

    send_started(9u);
    send_data(9u, 1u, &byte, 1u);
    assert(test_frame_return_count == 2u);
}
```

Also assert: irrelevant PROPERTY events are ignored, failed completion keeps the older complete frame, newer complete generation replaces an unconsumed older frame, an unavailable frame drops only the new generation, and callback code never calls JPEG decode or display draw.

- [ ] **Step 2: Run the new test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File `
  application/task_state_handling/state_bt_music/tests/test_bt_music_art.ps1
```

Expected: nonzero exit because `bt_music_art.h`/`bt_music_art.c` and their symbols do not exist.

- [ ] **Step 3: Add the internal header**

Use an application-private header; do not add it to BTstack packaged headers:

```c
#ifndef BT_MUSIC_ART_H
#define BT_MUSIC_ART_H

#include <stdint.h>
#include "bt_xstack.h"

#define BT_MUSIC_ART_MAX_JPEG_SIZE (64u * 1024u)

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} bt_music_art_rect_t;

void bt_music_art_state_entry(void *previous_state);
void bt_music_art_cover_event(
    const bt_xstack_cover_art_event_t *event, void *context);
int bt_music_art_calculate_fit(uint16_t source_width,
    uint16_t source_height, uint16_t screen_width,
    uint16_t screen_height, bt_music_art_rect_t *rect);
int bt_music_art_parse_jpeg_size(const uint8_t *data, uint32_t length,
    uint16_t *width, uint16_t *height);

#endif
```

- [ ] **Step 4: Implement the minimal receive/ownership state machine**

Use one private context and explicit ownership fields:

```c
typedef struct {
    DISP_P receiving_frame;
    uint32_t receiving_generation;
    uint32_t expected_offset;
    DISP_P cached_frame;
    uint32_t cached_generation;
    uint32_t cached_size;
    DISP_P decoding_frame;
    uint32_t decoding_generation;
    uint8_t receiving;
    uint8_t page_active;
} bt_music_art_context_t;
```

Implement event handling with these invariants:

```c
case BT_XSTACK_COVER_ART_STARTED:
    /* Ignore property-only sequences; acquire a display frame nonblocking,
       then install it as receiving. Release an incomplete older receiver. */
    break;

case BT_XSTACK_COVER_ART_DATA:
    if (!ctx.receiving || event->generation != ctx.receiving_generation ||
        event->data.data.offset != ctx.expected_offset ||
        event->data.data.length >
            BT_MUSIC_ART_MAX_JPEG_SIZE - ctx.expected_offset) {
        bt_music_art_abort_receive();
        break;
    }
    memcpy((uint8_t *)(uintptr_t)ctx.receiving_frame->base +
        ctx.expected_offset, event->data.data.data,
        event->data.data.length);
    ctx.expected_offset += event->data.data.length;
    break;

case BT_XSTACK_COVER_ART_COMPLETED:
    if (ctx.receiving && event->generation == ctx.receiving_generation &&
        event->status == BT_XSTACK_OK &&
        event->data.completed.total_length == ctx.expected_offset) {
        bt_music_art_publish_complete();
    } else {
        bt_music_art_abort_receive();
    }
    break;

case BT_XSTACK_COVER_ART_ERROR:
    bt_music_art_abort_receive();
    break;
```

Capture frame pointers under `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`
and perform queue put/message send after leaving it. Never hold a critical
section across `memcpy`, callback dispatch, JPEG decode, or queue notification.

- [ ] **Step 5: Run the receiver test and verify GREEN**

Run the Task 1 PowerShell script again.

Expected: `PASS: Bluetooth music Cover Art receiver` and exit code 0 with `-std=c99 -Wall -Wextra -Werror`.

- [ ] **Step 6: Record a checkpoint**

If the workspace has Git, commit only Task 1 files:

```bash
git add application/task_state_handling/state_bt_music
git commit -m "feat: add bounded Bluetooth cover receiver"
```

If `.git` is absent, do not initialize a repository; record Task 1 GREEN output in the execution log and continue.

### Task 2: Add the Bluetooth music state and key ownership

**Files:**
- Modify: `application/inc/turnkey_sys_msg.h`
- Modify: `application/inc/turnkey_drv_msg.h`
- Modify: `application/task_state_handling/src/task_state_handling.c`
- Modify: `application/task_state_handling/state_browse/src/state_browse.c`
- Modify: `application/task_peripheral_handling/inc/ap_peripheral_handling.h`
- Modify: `application/task_peripheral_handling/src/ap_peripheral_handling.c`
- Modify: `application/task_state_handling/state_bt_music/src/bt_music_art.c`
- Create: `application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1`

- [ ] **Step 1: Write the state/key integration contract**

The PowerShell test must assert all of the following from production sources:

```powershell
Assert-Contains $turnkeyMessages 'STATE_BT_MUSIC'
Assert-Contains $stateTask 'case STATE_BT_MUSIC:'
Assert-Contains $stateTask 'bt_music_art_state_entry'
Assert-Regex $browse 'case\s+MSG_APQ_MODE:[\s\S]*STATE_BT_MUSIC'
Assert-Regex $music 'case\s+MSG_APQ_MODE:[\s\S]*STATE_VIDEO_RECORD'
Assert-Regex $music 'MSG_APQ_FUNCTION_KEY_ACTIVE[\s\S]*bt_xstack_avrcp_play_pause'
Assert-Regex $music 'MSG_APQ_PREV_KEY_ACTIVE[\s\S]*bt_xstack_avrcp_previous'
Assert-Regex $music 'MSG_APQ_NEXT_KEY_ACTIVE[\s\S]*bt_xstack_avrcp_next'
Assert-NotContains $peripheral 'AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE'
Assert-NotContains $peripheral 'AP_PERIPHERAL_MEDIA_KEY_NEXT'
Assert-NotContains $peripheral 'AP_PERIPHERAL_MEDIA_KEY_PREVIOUS'
```

Also compile the header as C99 and C++11 and ensure no backup source is referenced by the generated project.

- [ ] **Step 2: Run the integration contract and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File `
  application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1
```

Expected: failure identifying missing `STATE_BT_MUSIC` and missing dispatch.

- [ ] **Step 3: Add the state and message identifiers**

Insert `STATE_BT_MUSIC` before `STATE_MAX` without changing `STATE_BASE`:

```c
STATE_SENSOR_SAVE_RAW,
STATE_BT_MUSIC,
STATE_MAX
```

Add `MSG_APQ_BT_COVER_READY` in the `MSG_APQ_*` enum; do not reuse a numeric value and do not encode a `DISP_P` in the message payload.

- [ ] **Step 4: Dispatch the new state and change the MODE cycle**

Add to `task_state_handling.c`:

```c
case STATE_BT_MUSIC:
    bt_music_art_state_entry((void *)&prev_state);
    break;
```

Change only the idle MODE transition in `state_browse.c`:

```c
state_handling_msg_send(STATE_BT_MUSIC, NULL, 0, MSG_PRI_NORMAL);
exit_flag = EXIT_BREAK;
```

Implement the new state loop so these messages have exact behavior:

```c
case MSG_APQ_MODE:
    state_handling_msg_send(STATE_VIDEO_RECORD, NULL, 0, MSG_PRI_NORMAL);
    exit_flag = EXIT_BREAK;
    break;
case MSG_APQ_FUNCTION_KEY_ACTIVE:
    (void)bt_xstack_avrcp_play_pause();
    break;
case MSG_APQ_PREV_KEY_ACTIVE:
    (void)bt_xstack_avrcp_previous();
    break;
case MSG_APQ_NEXT_KEY_ACTIVE:
    (void)bt_xstack_avrcp_next();
    break;
case MSG_APQ_BT_COVER_READY:
    bt_music_art_decode_latest();
    break;
```

Set `page_active` before scheduling a cached cover on entry and clear it before returning on exit. A stale ready message contains only a generation, so another state can safely ignore it.

On entry, call `bt_music_art_present_black()` before attempting a cached
cover. It nonblockingly acquires one output frame, fills the whole current TFT
buffer with the existing format-correct black pattern, and submits one
`MSG_DISPLAY_TASK_JPEG_DRAW`. If no frame is available, entry continues
without blocking. The Task 2 behavior test must assert one black draw when no
cover is cached and no second black draw during a normal cover replacement.

- [ ] **Step 5: Remove the global media-key interception**

In `ap_peripheral_mode_key_exe()`, `ap_peripheral_next_key_exe()`, and `ap_peripheral_menu_key_exe()`, remove only the early `ap_peripheral_media_key_dispatch()` blocks. Preserve screen-saver handling, audio effects, call-key handling, and the normal `MSG_APQ_*` sends.

Remove the now-unused `ap_peripheral_media_key_action_t`, handler typedef, registration function, static handler pointer, and static dispatch helper from the application peripheral header/source. Do not remove call-key registration.

- [ ] **Step 6: Run Task 1 and Task 2 tests**

Run both PowerShell scripts.

Expected: both exit 0; the behavior test must additionally simulate `MSG_APQ_MODE`, OK, PREV, and NEXT and observe one corresponding state/API action each.

- [ ] **Step 7: Record a checkpoint**

If Git exists:

```bash
git add application/inc application/task_state_handling application/task_peripheral_handling
git commit -m "feat: add Bluetooth music application state"
```

Otherwise record the two GREEN outputs without creating VCS metadata.

### Task 3: Implement fit-and-center JPEG decode

**Files:**
- Modify: `application/task_state_handling/state_bt_music/src/bt_music_art.c`
- Modify: `application/task_state_handling/state_bt_music/tests/test_bt_music_art.c`
- Read only: `application/task_decoder/src/ap_jpeg_h264.c`
- Read only: `application/task_display/src/ap_display_func.c`

- [ ] **Step 1: Add RED tests for fit rectangles and display replacement**

Use exact expected rectangles for a 320x240 screen:

```c
static void test_fit_rectangles(void)
{
    bt_music_art_rect_t rect;

    assert(bt_music_art_calculate_fit(200u, 200u, 320u, 240u,
        &rect) == 0);
    assert(rect.width == 240u && rect.height == 240u);
    assert(rect.x == 40u && rect.y == 0u);

    assert(bt_music_art_calculate_fit(400u, 200u, 320u, 240u,
        &rect) == 0);
    assert(rect.width == 320u && rect.height == 160u);
    assert(rect.x == 0u && rect.y == 40u);

    assert(bt_music_art_calculate_fit(100u, 200u, 320u, 240u,
        &rect) == 0);
    assert(rect.width == 120u && rect.height == 240u);
    assert(rect.x == 100u && rect.y == 0u);
}
```

Also assert: zero dimensions fail; JPEG decode is called only from the state message path; successful decode sends exactly one `MSG_DISPLAY_TASK_JPEG_DRAW`; decode failure, display enqueue failure, page exit, and unsupported format send none and return each newly acquired frame exactly once; an old displayed cover is never explicitly cleared during replacement.

Add baseline JPEG dimension cases so decode setup does not depend on a second
hardware parse just to calculate the rectangle:

```c
static const uint8_t jpeg_200x100[] = {
    0xff, 0xd8,
    0xff, 0xc0, 0x00, 0x0b, 0x08,
    0x00, 0x64, 0x00, 0xc8, 0x01, 0x01, 0x11, 0x00,
    0xff, 0xd9
};
uint16_t width = 0u;
uint16_t height = 0u;
assert(bt_music_art_parse_jpeg_size(jpeg_200x100,
    sizeof(jpeg_200x100), &width, &height) == 0);
assert(width == 200u && height == 100u);
```

Test truncated segments, zero dimensions, data without SOI, and SOS before SOF as failures.

- [ ] **Step 2: Run the behavior test and verify RED**

Expected: fit/decode assertions fail because the functions are not implemented.

- [ ] **Step 3: Implement overflow-safe fit calculation**

Use cross multiplication and align output width to the scaler's 8-pixel requirement:

```c
int bt_music_art_calculate_fit(uint16_t sw, uint16_t sh,
    uint16_t dw, uint16_t dh, bt_music_art_rect_t *rect)
{
    uint32_t width;
    uint32_t height;

    if (sw == 0u || sh == 0u || dw == 0u || dh == 0u || rect == NULL) {
        return -1;
    }
    if ((uint32_t)dw * sh <= (uint32_t)dh * sw) {
        width = dw;
        width &= ~7u;
        height = ((uint32_t)sh * width) / sw;
    } else {
        height = dh;
        width = ((uint32_t)sw * height) / sh;
        width &= ~7u;
        height = ((uint32_t)sh * width) / sw;
    }
    if (width == 0u || height == 0u) return -1;
    rect->width = (uint16_t)width;
    rect->height = (uint16_t)height;
    rect->x = (uint16_t)((dw - width) / 2u);
    rect->y = (uint16_t)((dh - height) / 2u);
    return 0;
}
```

- [ ] **Step 4: Implement deferred decode and centered composition**

In State Handling Task context:

1. claim the cached generation into `decoding_frame` under the short application critical section;
2. acquire an output `DISP_P` nonblocking;
3. query TFT width, height, format, and `getDispDevBufSize()`;
4. parse the bounded JPEG marker stream in `bt_music_art_parse_jpeg_size()` until baseline/extended sequential SOF (`0xc0`/`0xc1`), rejecting malformed lengths, zero dimensions, SOS-before-SOF, and truncation;
5. initialize the whole output frame to the existing black representation for RGB565/GP420/YUYV;
6. calculate the rectangle and configure `IMAGE_DECODE_STRUCT` with buffer source, current TFT format, destination dimensions, boundary color `0x008080`, and a centered destination address. For RGB565/YUYV use `base + 2 * (y * screen_width + x)`; for GP420 use `base + y * screen_width + x`, matching the existing display-row addressing. Set the decoder buffer width to the full screen stride and buffer height to `screen_height - y`;
7. call `jpeg_buffer_decode_and_scale()`;
8. on success, perform the existing cache maintenance and send `MSG_DISPLAY_TASK_JPEG_DRAW`;
9. return the compressed frame to cache only if it is still the newest generation; otherwise return it to `display_free_queue`.

Do not modify `ap_jpeg_h264.c` or enable the old `C_BROWSE_FULL_SCREEN` branch globally. `bt_music_art.c` supplies the already calculated rectangle and offset to the existing decoder.

- [ ] **Step 5: Run behavior tests and ARM syntax compilation**

Run the host test, then compile the new module with the main project's actual ARM include list and flags using `arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -fsyntax-only`.

Expected: host PASS and ARM syntax exit 0. If existing project headers produce unrelated warnings, keep `-Werror` on the new translation unit and report external warnings separately.

- [ ] **Step 6: Record a checkpoint**

If Git exists:

```bash
git add application/task_state_handling/state_bt_music
git commit -m "feat: decode and center Bluetooth album art"
```

Otherwise retain the host and ARM GREEN evidence.

### Task 4: Register Cover Art and wire the main project

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Regenerate: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk`
- Modify: `application/task_state_handling/state_bt_music/tests/test_bt_music_art_integration.ps1`

- [ ] **Step 1: Extend the integration contract and verify RED**

Require:

```powershell
Assert-Contains $demo 'config.cover_art_callback = bt_music_art_cover_event;'
Assert-Contains $demo 'BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL'
Assert-NotContains $demo 'bt_xstack_demo_media_key_handler'
Assert-ProjectUnitOnce $project '../../application/task_state_handling/state_bt_music/src/bt_music_art.c'
Assert-GeneratedCompileRuleOnce $make 'state_bt_music\\src\\bt_music_art.o'
```

Expected initial failure: callback assignment and project unit are absent.

- [ ] **Step 2: Register the callback and mode**

Add the include and callback assignment before `bt_xstack_init()`:

```c
#include "bt_music_art.h"

config.cover_art_callback = bt_music_art_cover_event;
```

After `bt_xstack_enable()` returns `BT_XSTACK_OK`, explicitly enqueue the mode
selection so it follows enable in the API request FIFO:

```c
status = bt_xstack_cover_art_set_mode(
    BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL);
if (status != BT_XSTACK_OK) {
    bt_xstack_demo_set_start_state(BT_XSTACK_DEMO_IDLE);
    bt_audio_task_stop();
    return status;
}
```

Remove `bt_xstack_demo_media_key_handler()` and its registration because the active state now owns media controls. Keep call-key registration.

- [ ] **Step 3: Add the source/include to the official main project**

Add one `<Unit>` for `bt_music_art.c` beside other state modules and add the new `inc` directory once to the Debug target include list. Do not add the source to BTstack API, AT, or legacy library projects.

- [ ] **Step 4: Regenerate the Debug makefile**

Run from `Project/GPCV2178_Turnkey_SDK`:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.exe' `
  -in 'GPCV2178_turnkey_sdk_release.gpcmprj' `
  -cfg 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.cfg' `
  -out 'GPCV2178_turnkey_sdk_release_Debug.mk' `
  -tp 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin' `
  -windows -targets Debug --with-deps --quote-path auto
if ($LASTEXITCODE -ne 0) { throw 'main makefile generation failed' }
```

Expected: generator exit 0 and exactly one compile rule for `bt_music_art.c`.

- [ ] **Step 5: Run all focused tests**

Run receiver behavior and integration contracts. Also run the existing focused BTstack Cover Art/API tests found with:

```powershell
rg --files btstack | rg 'cover_art|bip|api_contract.*ps1$'
```

Expected: all selected scripts exit 0 and no packaged BTstack header changes.

- [ ] **Step 6: Record a checkpoint**

If Git exists:

```bash
git add Project/wifi_demo/bt_xstack_demo.c \
  Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj \
  Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk \
  application/task_state_handling/state_bt_music/tests
git commit -m "feat: integrate Bluetooth music cover display"
```

Otherwise record generator and focused-test outputs.

### Task 5: Fresh target verification and handoff

**Files:**
- Verify only: all changed production/project/test files

- [ ] **Step 1: Run the focused suite in fresh processes**

Run every new PowerShell test in a separate `powershell -NoProfile` process, followed by existing Cover Art/BIP, API demo, display/JPEG, and main build-integration contracts relevant to the changed files. Stop on the first production-related failure; classify unrelated known baseline failures explicitly rather than hiding them.

- [ ] **Step 2: Fresh-build the BTstack API library**

Because the main application links it and the Cover Art callback/API must remain compatible:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File `
  Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1 -Jobs 4
```

Expected: exit 0; no new Cover Art, platform, or application warning.

- [ ] **Step 3: Fresh-build the main firmware**

From `Project/GPCV2178_Turnkey_SDK`:

```powershell
$env:Path = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;' +
  'D:\mingw32\bin;' + $env:Path
& 'D:\mingw32\bin\mingw32-make.exe' -B `
  -f 'GPCV2178_turnkey_sdk_release_Debug.mk' -j4
if ($LASTEXITCODE -ne 0) { throw 'main firmware build failed' }
```

Expected: compile and final link exit 0. Report existing SDK warnings separately and require zero warning whose path contains `bt_music_art`, `bt_xstack_demo`, or the changed peripheral source.

- [ ] **Step 4: Audit the final ELF/map**

Use ARM `nm`, `objdump`, and the map file to verify:

- `bt_music_art_state_entry` and `bt_music_art_cover_event` each have one definition;
- `bt_xstack_cover_art_set_mode` and the Cover Art delivery path are linked;
- `bt_music_art.c` is present once;
- no new task entry, queue object, 64/128 KiB static JPEG array, `malloc`, or filesystem symbol was introduced by the module;
- the final ELF and map are newer than the changed application source and API archive.

- [ ] **Step 5: Perform CN1 board validation**

On the target board:

1. pair Android, enter Bluetooth music via MODE, start playback;
2. verify a square cover is fully visible with symmetric black side bars;
3. repeat with landscape and portrait art;
4. press OK/PREV/NEXT and verify play/pause/track control;
5. switch tracks quickly and verify only a fully decoded cover replaces the old one;
6. disconnect/reconnect and verify no crash, half frame, or stuck black refresh;
7. leave via MODE and verify video record reclaims the display.

Record the phone model/OS, display resolution/format, image sizes, and observed result. Host and cross-build evidence cannot replace this physical CN1 check.

- [ ] **Step 6: Final checkpoint**

If Git exists and all required verification is green:

```bash
git add application Project docs/superpowers
git commit -m "test: verify Bluetooth album art display"
```

If Git is absent, do not initialize it; deliver the changed-file list, exact test/build commands and results, artifact timestamps/hashes, warnings, and the explicit board-test status.
