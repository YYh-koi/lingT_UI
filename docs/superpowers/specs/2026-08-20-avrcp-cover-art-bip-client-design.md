# AVRCP Cover Art and BIP Image Client Design

## Goal

Add album-art retrieval for the currently playing phone track without storing a
complete image in the Bluetooth stack. The implementation supports two runtime
modes in both API and public AT builds:

- automatic AVRCP linked-thumbnail download;
- automatic image-properties discovery followed by an application-selected
  BIP `GetImage` request.

Legacy `BTstack_Library` is not synchronized by this change.

## Scope

The first release implements the AVRCP Cover Art subset of BIP:

- `GetLinkedThumbnail`;
- `GetImageProperties`;
- `GetImage` with an application-provided Image Descriptor.

It does not implement image-list browsing, image upload, delete, print,
automatic archive, referenced objects, or remote-camera operations.

The default runtime mode is linked thumbnail. The selected mode may be changed
at runtime. A mode change cancels the old operation and advances a generation
so late data from the old operation cannot reach the application.

## Layering and Ownership

### Native protocol layer

Restore the official BTstack v1.6.2 source:

```text
btstack/src/classic/avrcp_cover_art_client.c
```

The native source owns AVRCP Cover Art SDP discovery, L2CAP ERTM, GOEP, OBEX,
SRM handling, BIP request encoding, native connection identifiers, and raw
`BIP_DATA_PACKET` delivery. The matching native header already exists. Native
protocol names remain professional and are not exposed as application policy.

### BIP operation layer

Add:

```text
btstack/profiles/btstack_bip_client.c
btstack/include/profiles/btstack_bip_client.h
```

This module is the sole owner of the project-level BIP session and operation
state. It serializes operations, maps native status values, incrementally
parses image-properties XML, counts image bytes, and emits normalized internal
events. It exposes project-internal operations for:

- linked thumbnail, used by the AVRCP coordinator;
- image properties;
- image retrieval with an Image Descriptor;
- cancellation and peer disconnect cleanup.

Only one BIP operation may be active at a time. A second operation returns
`BUSY`; it never implicitly steals an active operation. Track change, mode
change, AVRCP disconnect, and peer disconnect are explicit cancellation
conditions.

### AVRCP automatic policy layer

Add:

```text
btstack/profiles/btstack_avrcp_cover_art.c
btstack/include/profiles/btstack_avrcp_cover_art.h
```

This module owns current-track cover-art policy. It receives the seven-byte
`AVRCP_MEDIA_ATTR_DEFAULT_COVER_ART` handle from the AVRCP Controller wrapper,
validates and copies it, tracks the current generation, and selects the BIP
operation according to the runtime mode:

- `LINKED_THUMBNAIL`: automatically start `GetLinkedThumbnail`;
- `IMAGE`: automatically start `GetImageProperties`, then wait for the
  application to select an Image Descriptor and request `GetImage`.

It does not parse GOEP/OBEX packets and does not contain frontend-specific
logic.

### Profile Manager and Event Mapper

The Profile Manager initializes both modules when Classic, AVRCP Controller,
and Cover Art are enabled. It receives normalized Cover Art/BIP events and
passes them to the Event Mapper. It may update aggregate state but does not
parse XML, buffer image data, or issue native OBEX commands.

The Event Mapper performs frontend-neutral conversion only. Image bytes do not
enter the ordinary API critical/best-effort event rings or the AT Event Ring.
The mapper calls a frontend delivery boundary directly, following the existing
direct-media-data pattern.

## Feature Selection

The canonical `btstack/include/btstack_config.h` adds:

```c
#define BTSTACK_AVRCP_COVER_ART 1
```

The implementation is active only when all required parents are active. The
BTstack v1.6.2 native Cover Art client requires both local AVRCP role
connections, so this first integration keeps that upstream requirement:

```c
#if BTSTACK_CLASSIC && \
    BTSTACK_AVRCP_CONTROLLER && \
    BTSTACK_AVRCP_TARGET && \
    BTSTACK_AVRCP_COVER_ART
```

The canonical project macro derives the upstream
`ENABLE_AVRCP_COVER_ART` integration macro. Project-owned code uses only the
canonical `BTSTACK_*` selection macros.

When disabled, public Cover Art functions remain linkable and return the
project's established unsupported/default result. Disabled code does not
allocate the ERTM buffer or BIP state.

## Runtime Modes

```c
typedef enum {
    BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL = 0,
    BT_XSTACK_COVER_ART_MODE_IMAGE
} bt_xstack_cover_art_mode_t;
```

The default is `LINKED_THUMBNAIL`.

### Linked thumbnail mode

1. AVRCP connects and requests now-playing metadata, including the default
   cover-art attribute.
2. A valid seven-byte handle is copied into the Cover Art coordinator.
3. The coordinator opens or reuses the BIP session.
4. `GetLinkedThumbnail` starts automatically.
5. JPEG bytes are delivered in order as transient callback chunks.
6. Completion reports the final byte count and status.

### Image mode

1. The handle-discovery path is identical.
2. `GetImageProperties` starts automatically.
3. XML is parsed incrementally. Each native image or variant entry is reported
   separately without storing the complete XML document.
4. The application selects an entry, builds an Image Descriptor, and calls the
   public `GetImage` API.
5. Image bytes are delivered with the same streaming callback used for the
   linked thumbnail.

The BIP session may remain connected for the current AVRCP peer between
operations. AVRCP disconnect, Cover Art service loss, or terminal protocol
failure closes it.

## Public API

Add a public Cover Art header to both the API and public AT include surfaces.
Application-facing names use Cover Art terminology; BIP names remain internal.

The API provides:

```c
bt_xstack_result_t bt_xstack_cover_art_set_mode(
    bt_xstack_cover_art_mode_t mode);

bt_xstack_result_t bt_xstack_cover_art_get_image(
    const bt_xstack_cover_art_image_descriptor_t *descriptor);
```

`GetImageProperties` is automatic in image mode. `GetImage` is valid only after
properties for the current generation have completed. The implementation
builds the XML Image Descriptor into a bounded local request buffer; the
application does not pass a raw unbounded XML string.

API builds add a Cover Art callback to `bt_xstack_config_t`. Public AT builds
add a binary Cover Art callback registration function plus AT commands for mode
selection and image selection. JPEG bytes are never hex- or Base64-encoded and
never inserted into textual AT responses.

## Event Model

Use one event type with a union so unrelated payloads do not occupy memory at
the same time:

```c
typedef enum {
    BT_XSTACK_COVER_ART_STARTED = 0,
    BT_XSTACK_COVER_ART_PROPERTY,
    BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED,
    BT_XSTACK_COVER_ART_DATA,
    BT_XSTACK_COVER_ART_COMPLETED,
    BT_XSTACK_COVER_ART_ERROR
} bt_xstack_cover_art_event_type_t;
```

Event payloads contain only the fields relevant to the selected type:

- `STARTED`: peer address, image handle, operation, and generation;
- `PROPERTY`: native/variant kind, encoding, pixel width/height or bounded
  range representation, file size when present, and transformation flags;
- `PROPERTIES_COMPLETED`: property count and generation;
- `DATA`: transient data pointer, chunk length, byte offset, and generation;
- `COMPLETED`: total bytes, status, and generation;
- `ERROR`: operation, mapped error, native status, and generation.

The data pointer is valid only for the duration of the callback. The callback
runs in Stack Run Loop context and must be bounded and non-blocking. An
application that writes flash, filesystem, display memory, or another slow
sink must copy the chunk before returning and perform the slow work elsewhere.

No callback is registered means no automatic Cover Art request is started.
Registering a callback while a current valid handle is cached starts the
selected mode for that handle.

## Image Properties Parsing

The BIP image-properties document is parsed incrementally with a fixed parser
state. The implementation does not retain the document or allocate memory.
Each `<native>` or `<variant>` entry produces one `PROPERTY` event.

Bounded fields are used for encoding and transformation text. Numeric pixel
and size fields are parsed with overflow checks. Unsupported or malformed
entries generate a protocol error and terminate the properties operation;
they are not partially reused to build a later Image Descriptor.

The public Image Descriptor structure represents the selected encoding,
width/height, and optional transformation with fixed-size fields. Descriptor
serialization checks the final bounded request length before starting
`GetImage`.

## Memory and Concurrency

- No complete image buffer.
- No complete XML buffer.
- No new worker, task, queue, or dynamic allocation.
- One statically allocated native Cover Art client object.
- One conditionally compiled ERTM buffer, initially sized to the official
  v1.6.2 example requirement and measured in the final API/AT artifacts.
- One seven-byte handle plus terminator.
- Fixed parser, descriptor, event, and generation state.

Native data is delivered synchronously and is not retained after the callback.
All state transitions occur on the Stack Run Loop. Frontend callback pointers
and mode selection shared with application tasks use the existing platform
critical-section boundary. User callbacks are always invoked outside that
critical section.

## Cancellation and Stale Data

Every accepted track handle advances a nonzero generation. Events carry the
generation that created them. On track change, mode change, disconnect, or
cancel:

1. mark the old generation canceled;
2. request orderly BIP/GOEP disconnect;
3. ignore late native data and terminal events from the old generation;
4. after cleanup, start only the latest pending handle/mode;
5. emit at most one terminal event for each started public operation.

Repeated notifications for the same handle and generation are deduplicated.
Failure to connect, disconnect, or execute an OBEX request must not leave the
module permanently busy.

## Build Integration

Update and rebuild only:

```text
Project/WirelessLibraries/BTstack_API_Library
Project/WirelessLibraries/BTstack_AT_Library
```

Do not update `Project/WirelessLibraries/BTstack_Library` in this change. API
and AT remain mutually selected builds. Both selected projects include the
native Cover Art source, the BIP wrapper, and the AVRCP automatic coordinator
exactly once. Packaged API/AT headers must match their source headers.

## Error Semantics

Public errors distinguish at least:

- unsupported remote Cover Art service;
- no current image handle;
- busy operation;
- invalid mode or Image Descriptor;
- SDP, L2CAP, GOEP, OBEX, parsing, disconnect, and peer-loss failures;
- stale/canceled generation.

Automatic thumbnail or properties retrieval may perform one bounded reconnect
retry after a clean terminal failure. It never retries in a tight loop. A new
track or explicit mode change resets the retry budget.

## Verification

Host behavior tests cover:

1. official v1.6.2 native source integration and Cover Art SDP/operation events;
2. seven-byte handle validation and copying;
3. automatic linked-thumbnail flow and ordered transient chunks;
4. automatic image-properties flow with multiple native/variant entries;
5. bounded XML parsing, malformed XML, numeric overflow, and descriptor build;
6. `GetImage` with the selected descriptor;
7. one-operation-at-a-time `BUSY` behavior;
8. track/mode/disconnect cancellation, stale generation filtering, and exactly
   one terminal event;
9. API callback and public AT binary callback behavior;
10. callback-null suppression and late callback registration;
11. compile matrix for parent/profile macros and disabled stubs;
12. absence of image/XML whole-object buffers, allocation, new workers, and
    binary data in textual event rings.

Official API and AT projects are regenerated and rebuilt. Archive checks prove
the three new production objects and public symbols are present exactly once,
Legacy was not rebuilt, packaged headers match, and no new warning is produced
by Cover Art/BIP code. Final target interoperability still requires Android and
iPhone testing for service discovery, handles, image-property variants,
thumbnail/image download, track changes, disconnects, and application callback
timing.
