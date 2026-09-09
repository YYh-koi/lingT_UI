# Bluetooth Music Cover Display Design

## Goal

Display the phone's current album cover on the CN1-connected screen while the
application is in a dedicated Bluetooth music page. The image comes from the
existing AVRCP Cover Art linked-thumbnail flow, is decoded by the existing
JPEG hardware path, and is shown complete, aspect-ratio preserving, centered,
and without cropping.

The design must keep Bluetooth callbacks bounded, avoid a new task, avoid a
new large permanent allocation, preserve the previous valid cover across
temporary failures, and leave the public BTstack API/AT ABI unchanged.

## Scope

This change adds application integration for the API build:

- a `STATE_BT_MUSIC` application state;
- MODE-key navigation into and out of that state;
- automatic reception of `GetLinkedThumbnail` data through the existing
  `bt_xstack_cover_art_callback_t` callback;
- bounded JPEG accumulation using display buffers already owned by the
  application;
- JPEG decode, fit-and-center composition, and Display Task submission;
- AVRCP play/pause, previous, and next controls on the Bluetooth music page.

It does not add a new Bluetooth protocol, alter the BIP client, change the
public Cover Art event types, display image properties, add a filesystem
cache, add a dedicated Cover Art task, or make API and AT frontends coexist in
one firmware image.

The existing Cover Art/BIP design remains the protocol-level source of truth:
`2026-08-20-avrcp-cover-art-bip-client-design.md`. This design only defines
the application/display consumer.

## Existing Constraints

- Cover Art data callbacks run in Stack Run Loop context. Data pointers are
  transient and must be copied before the callback returns.
- JPEG decoding is synchronous and must not run in Stack Run Loop context.
- The existing State Handling Task owns application page behavior.
- The existing Display Task owns final frame presentation through
  `MSG_DISPLAY_TASK_JPEG_DRAW`.
- The active board configuration uses TFT2. Width, height, pixel format, and
  display-buffer size are still queried at runtime rather than hard-coded.
- A linked thumbnail larger than 64 KiB is outside the accepted application
  bound and is discarded.

## Application State and Navigation

Add `STATE_BT_MUSIC` to `application/inc/turnkey_sys_msg.h` and dispatch it
from `application/task_state_handling/src/task_state_handling.c`.

The normal MODE cycle becomes:

```text
video record -> photo capture -> audio record -> browse
             -> Bluetooth music -> video record
```

Build-time-disabled states remain skipped in the same way as today. The
specific transition changed by this feature is browse to Bluetooth music; the
Bluetooth music page transitions to video record.

The current peripheral MODE handler first tries to treat MODE as Bluetooth
play/pause. That precedence is removed. Key ownership becomes:

- MODE: application page transition;
- OK while in `STATE_BT_MUSIC`: AVRCP play/pause;
- PREV while in `STATE_BT_MUSIC`: AVRCP previous track;
- NEXT while in `STATE_BT_MUSIC`: AVRCP next track.

Other states retain their existing OK/PREV/NEXT behavior.

## Files and Internal Interfaces

Add one application module, named as requested:

```text
application/task_state_handling/state_bt_music/inc/bt_music_art.h
application/task_state_handling/state_bt_music/src/bt_music_art.c
```

The module owns both the Bluetooth music page and its cover-art consumer. Its
small internal surface is:

```c
void bt_music_art_state_entry(void *previous_state);

void bt_music_art_cover_event(
    const bt_xstack_cover_art_event_t *event,
    void *context);
```

`Project/wifi_demo/bt_xstack_demo.c` registers
`bt_music_art_cover_event()` in `bt_xstack_config_t`, explicitly selects
`BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL`, and otherwise remains unaware of
JPEG decoding and display ownership.

Add an application-private message such as `MSG_APQ_BT_COVER_READY` to the
existing application message namespace. The message is a wakeup, not an
ownership transfer. Its payload contains only the generation:

```c
typedef struct {
    uint32_t generation;
} bt_music_art_ready_t;
```

The frame pointer and length remain in module-private state. This makes a late
message harmless after a page transition: another state may ignore the stale
message, while `bt_music_art.c` still has enough ownership information to
retain or release the frame correctly.

The module is added only to the main application project. BTstack API and AT
library public headers and ABIs do not change.

## Cover Art Reception

The consumer handles only the linked-thumbnail data path:

### STARTED

- The application is explicitly configured in linked-thumbnail mode; accept
  its image-data sequence and ignore property-only event types. Do not compare
  `started.operation` against a private BIP enum that is absent from the public
  API.
- Release any incomplete older receive frame.
- Non-blockingly obtain one frame from `display_free_queue`.
- Record the event generation, reset the expected offset and byte count, and
  mark the frame as receiving.
- If no frame is available or its writable capacity is below the required
  bound, mark this generation discarded without disturbing the displayed
  cover.

### DATA

- Require a currently accepted generation.
- Require `event.data.offset == expected_offset`.
- Require `offset + length <= 64 KiB` and within the actual borrowed frame
  capacity.
- Copy the transient callback bytes into the borrowed frame and advance the
  expected offset.
- On any violation, release the incomplete frame and discard the generation.

### COMPLETED

- Require matching generation, success status, and
  `total_length == accumulated_length`.
- Transfer frame ownership from the receiver to the latest-complete slot.
- If an older complete frame has not started decoding, return it to
  `display_free_queue`; newest generation wins.
- If the Bluetooth music page is active, send `MSG_APQ_BT_COVER_READY` with
  only the generation.
- If the page is inactive, retain at most the newest compressed frame so it
  can be displayed immediately on the next entry.

### ERROR or disconnect

- Release an incomplete receive frame.
- Do not clear the currently displayed valid cover.
- Do not replace a cached complete cover with an error.

Only short pointer/state updates use the application's existing
`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` boundary. JPEG decode, display
submission, callbacks, and queue notifications never run inside that critical
section.

## Ownership Model

Each borrowed display frame has exactly one owner or one explicitly tracked
borrower:

```text
display_free_queue
    -> Cover Art receiver
    -> latest complete cache
    -> State Handling Task decode borrow
    -> latest complete cache or display_free_queue
```

The compressed JPEG is stored in a borrowed display frame rather than a new
64/128 KiB static array. The 64 KiB value is the accepted image-size limit,
not a new permanent allocation.

Normally these resources are held by the feature:

- one frame receiving the current thumbnail;
- one latest complete cached frame, which may be borrowed by decode;
- one normal output frame acquired during decode.

If a new transfer completes while the old cached frame is being decoded, the
old frame is marked stale and released when that decode returns; the new frame
becomes the latest cache. This can transiently hold one stale decode source in
addition to the normal set, but remains bounded by the existing display-frame
pool. If no frame is available, the new transfer is discarded rather than
stealing or corrupting an owned frame. Failure paths return every frame
exactly once.

## Bluetooth Music Page

### Entry

- Register normal application keys.
- Stop/leave the previous state's display producer through its existing exit
  path.
- Clear the screen to black.
- Mark the page active.
- If a complete cached cover exists, enqueue it for decode immediately.

If no cover has ever completed, the page remains plain black. No placeholder
text, icon, or new resource is added.

### Active page

- Handle `MSG_APQ_BT_COVER_READY` in State Handling Task context.
- Ignore stale generations.
- Borrow the module's latest compressed frame for the selected generation and
  acquire an output frame from `display_free_queue`.
- Decode and submit only when the page is still active.
- Handle OK/PREV/NEXT by calling the existing public
  `bt_xstack_avrcp_play_pause()`, `bt_xstack_avrcp_previous()`, and
  `bt_xstack_avrcp_next()` functions from State Handling Task context.

The old application-global media-key registration/dispatch shortcut is
removed from the peripheral/demo integration because it intercepts keys before
the active state can assign their meaning. Call-key registration remains
unchanged and retains its existing answer/hang-up precedence.

### Exit

- Mark the page inactive before relinquishing display ownership.
- Return any page-owned compressed frame that is no longer the latest cache.
- Do not allow a late decode completion to submit a frame after exit.
- Transition to `STATE_VIDEO_RECORD`; that state reinitializes its own display.

## JPEG Decode and Composition

Decode uses the existing JPEG hardware implementation in
`application/task_decoder/src/ap_jpeg_h264.c`, but the Bluetooth music helper
must explicitly calculate a fit rectangle instead of relying on full-screen
stretch behavior.

For source dimensions `(source_width, source_height)` and screen dimensions
`(screen_width, screen_height)`:

```text
scale = min(screen_width / source_width,
            screen_height / source_height)

destination_width  = source_width  * scale
destination_height = source_height * scale
destination_x = (screen_width  - destination_width)  / 2
destination_y = (screen_height - destination_height) / 2
```

The integer implementation uses overflow-safe cross multiplication and the
alignment required by the JPEG/scaler output format. It may scale a small
thumbnail up, but it never changes aspect ratio and never crops.

Before decode, the full output frame is initialized to the correct black value
for the current TFT format. The decoder writes only the centered destination
rectangle. Address and stride calculations use the runtime TFT format; an
unsupported format fails without replacing the displayed cover.

On successful decode:

1. clean/invalidate caches as required by the existing display path;
2. send the output frame through `MSG_DISPLAY_TASK_JPEG_DRAW`;
3. return the compressed source frame to the latest-complete cache if its
   generation is still current; otherwise return it to `display_free_queue`.

On decode failure, remove that invalid compressed frame from the cache and
return it together with the unused output frame. On display-message failure,
return the output frame but keep a valid compressed source cached for retry.
In both cases the old displayed frame remains unchanged.

## Concurrency and Ordering

- Stack Run Loop context only validates and copies bounded chunks.
- State Handling Task performs all JPEG decode work.
- Display Task performs final presentation.
- Generation checks prevent old track data from replacing a newer cover.
- A new cover becomes visible only after full receipt and successful decode.
- The previous displayed cover remains visible during download and decode.
- Leaving `STATE_BT_MUSIC` invalidates any pending display submission without
  blocking the Bluetooth stack.

No new worker, task, queue, polling loop, heap-growing image buffer, or file is
introduced.

## Error Handling

The following conditions discard only the affected new cover:

- no free display frame;
- frame capacity below the received image requirement;
- non-JPEG or malformed JPEG;
- out-of-order or duplicate chunk offset;
- generation mismatch;
- length over 64 KiB;
- unsuccessful Cover Art terminal status;
- JPEG/scaler resource unavailable;
- state message or display message enqueue failure;
- page exit during pending decode.

The last successfully displayed cover is preserved. A Bluetooth disconnect
aborts incomplete reception but does not blank the page; the cover disappears
when the user leaves the Bluetooth music page or a later valid cover replaces
it.

## Verification

### Host and contract tests

- linked-thumbnail STARTED/DATA/COMPLETED assembly;
- exact 64 KiB acceptance and 64 KiB + 1 rejection;
- offset, total-length, status, generation, and irrelevant-property-event
  validation;
- newest-complete replacement and exact frame-return accounting;
- callback performs no decode and no blocking wait;
- `browse -> Bluetooth music -> video record` MODE sequence;
- MODE is no longer consumed as global play/pause;
- OK/PREV/NEXT mapping only in Bluetooth music state;
- no display submission outside the active page;
- decode failure and queue failure preserve the old cover;
- horizontal and vertical fit-rectangle calculations;
- no new public BTstack ABI and no new application task.

### Target build

- regenerate the main project makefile if its project manifest changes;
- fresh compile/link the main firmware;
- confirm the new state module and existing Cover Art API symbols are linked
  exactly once;
- confirm no new compiler warning in the changed application/JPEG path.

### CN1 hardware checks

- Android and iPhone linked-thumbnail reception;
- square, landscape, and portrait covers;
- centered black bars with no crop or distortion;
- rapid track changes;
- play/pause, previous, next, and MODE behavior;
- disconnect/reconnect, malformed/oversize image, and temporary buffer
  exhaustion;
- no half-frame display, flashing black between valid covers, frame leak, or
  Stack Run Loop stall.

## Acceptance Criteria

- The Bluetooth music page is reachable through MODE and exits through MODE.
- The page displays linked thumbnails completely, centered, and with preserved
  aspect ratio.
- No image larger than 64 KiB is accepted.
- Bluetooth callbacks do not decode, block, or directly draw.
- A failed new cover never replaces the previous valid cover.
- All borrowed frames have deterministic ownership and are returned exactly
  once on every failure path.
- No dedicated Cover Art task, unbounded allocation, public ABI change, or AT
  integration is introduced.
