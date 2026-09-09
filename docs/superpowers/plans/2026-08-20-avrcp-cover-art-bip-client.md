# AVRCP Cover Art and BIP Image Client Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add automatic current-track album-art retrieval with linked-thumbnail and negotiated-image modes to the mutually exclusive API and public AT builds.

**Architecture:** Restore the official BTstack v1.6.2 native AVRCP Cover Art client, wrap its single GOEP/OBEX session in `btstack_bip_client.c`, and keep automatic current-track policy in `btstack_avrcp_cover_art.c`. Profile Manager routes normalized events through Event Mapper to direct API/AT binary callbacks; image bytes and XML never enter ordinary event rings or whole-object buffers.

**Tech Stack:** C99, BTstack 1.6.2 AVRCP/GOEP/OBEX/L2CAP ERTM, yxml, fixed storage, MinGW host tests, ARM GCC GPCV2178 builds, PowerShell contracts.

---

## Preconditions and File Map

The approved design is
`docs/superpowers/specs/2026-08-20-avrcp-cover-art-bip-client-design.md`.
The current workspace has no `.git` directory. Each commit step below is a
checkpoint: run the shown Git commands only if execution occurs in a Git
worktree; otherwise record the checkpoint in the task log and continue without
inventing repository history.

### New production files

- `btstack/src/classic/avrcp_cover_art_client.c`: exact native BTstack v1.6.2 protocol implementation.
- `btstack/include/profiles/btstack_bip_client.h`: internal normalized BIP operation/event contract.
- `btstack/profiles/btstack_bip_client.c`: one BIP session, property parsing, operation serialization.
- `btstack/include/profiles/btstack_avrcp_cover_art.h`: automatic Cover Art coordinator contract.
- `btstack/profiles/btstack_avrcp_cover_art.c`: handle generation, runtime mode, automatic operation policy.
- `btstack/include/api/bt_xstack_cover_art.h`: API public Cover Art types/functions.
- `btstack/include/core/bt_xstack_cover_art.h`: frontend-neutral copy of the API contract.
- `btstack/include/at/bt_xstack_cover_art.h`: public AT binary Cover Art types/functions.

### New tests

- `btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1`
- `btstack/profiles/tests/test_btstack_bip_client.c`
- `btstack/profiles/tests/test_btstack_bip_client.ps1`
- `btstack/profiles/tests/test_btstack_avrcp_cover_art.c`
- `btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1`
- `btstack/api/tests/test_bt_xstack_cover_art.c`
- `btstack/api/tests/test_bt_xstack_cover_art_header.c`
- `btstack/api/tests/test_bt_xstack_cover_art_header.cpp`
- `btstack/api/tests/test_bt_xstack_cover_art.ps1`
- `btstack/at/tests/test_btstack_at_cover_art.c`
- `btstack/at/tests/test_btstack_at_cover_art.ps1`
- `btstack/core/tests/test_btstack_cover_art_build_contract.ps1`

### Existing files modified by responsibility

- `btstack/include/btstack_config.h`: canonical feature switch and upstream macro derivation.
- `btstack/src/classic/avrcp_controller.c`: already emits the native handle event when enabled; no policy is added.
- `btstack/profiles/btstack_avrcp_controller.c`: consume native handle subevent and notify the coordinator.
- `btstack/core/btstack_profile_manager.c`: initialize Cover Art and route normalized events.
- `btstack/core/btstack_event_mapper.c`: map normalized Cover Art events and call direct frontend delivery.
- `btstack/core/btstack_core_internal.h`: request IDs/payload and direct delivery declarations.
- `btstack/core/btstack_core_dispatcher.c`: execute mode/image requests on Stack Run Loop.
- `btstack/api/bt_xstack_api_core.c`: public API admission and callback delivery.
- `btstack/at/btstack_at_internal.h`: two AT command IDs and bounded parsed descriptor fields.
- `btstack/at/btstack_at_command_table.c`: `COVERMODE` and `COVERIMAGE` syntax.
- `btstack/at/btstack_at_dispatch_table.c`: route Cover Art commands.
- `btstack/at/btstack_at_parser.c`: parse bounded mode and Image Descriptor tokens.
- `btstack/at/btstack_at_core_adapter.c`: submit Cover Art requests and deliver the binary callback.
- `btstack/include/api/bt_xstack.h`, `btstack/include/core/bt_xstack.h`, `btstack/include/at/bt_xstack.h`: expose callback/config/header.
- `btstack/Makefile.inc`: selected Cover Art source group; no Legacy selection.
- API/AT `.gpcmprj` and generated `*_Debug.mk`: include sources exactly once.

### Stable public shapes used by all later tasks

```c
typedef enum {
    BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL = 0,
    BT_XSTACK_COVER_ART_MODE_IMAGE
} bt_xstack_cover_art_mode_t;

typedef enum {
    BT_XSTACK_COVER_ART_ENCODING_UNKNOWN = 0,
    BT_XSTACK_COVER_ART_ENCODING_JPEG,
    BT_XSTACK_COVER_ART_ENCODING_PNG,
    BT_XSTACK_COVER_ART_ENCODING_GIF,
    BT_XSTACK_COVER_ART_ENCODING_BMP,
    BT_XSTACK_COVER_ART_ENCODING_WBMP
} bt_xstack_cover_art_encoding_t;

typedef enum {
    BT_XSTACK_COVER_ART_TRANSFORMATION_NONE = 0,
    BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH = 1u << 0,
    BT_XSTACK_COVER_ART_TRANSFORMATION_FILL = 1u << 1,
    BT_XSTACK_COVER_ART_TRANSFORMATION_CROP = 1u << 2,
    BT_XSTACK_COVER_ART_TRANSFORMATION_ALL =
        BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH |
        BT_XSTACK_COVER_ART_TRANSFORMATION_FILL |
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP
} bt_xstack_cover_art_transformation_t;

typedef struct {
    bt_xstack_cover_art_encoding_t encoding;
    uint16_t width;
    uint16_t height;
    uint8_t transformation;
} bt_xstack_cover_art_image_descriptor_t;

typedef struct {
    bt_xstack_cover_art_encoding_t encoding;
    uint16_t min_width;
    uint16_t max_width;
    uint16_t min_height;
    uint16_t max_height;
    uint32_t file_size;
    uint8_t file_size_known;
    uint8_t native_image;
    uint8_t transformations;
} bt_xstack_cover_art_property_t;

typedef enum {
    BT_XSTACK_COVER_ART_STARTED = 0,
    BT_XSTACK_COVER_ART_PROPERTY,
    BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED,
    BT_XSTACK_COVER_ART_DATA,
    BT_XSTACK_COVER_ART_COMPLETED,
    BT_XSTACK_COVER_ART_ERROR
} bt_xstack_cover_art_event_type_t;

typedef struct {
    bt_xstack_cover_art_event_type_t type;
    int32_t status;
    uint32_t generation;
    union {
        struct { uint8_t address[6]; char image_handle[8]; uint8_t operation; } started;
        bt_xstack_cover_art_property_t property;
        struct { uint16_t property_count; } properties_completed;
        struct { const uint8_t *data; uint16_t length; uint32_t offset; } data;
        struct { uint32_t total_length; } completed;
        struct { uint8_t operation; uint8_t native_status; } error;
    } data;
} bt_xstack_cover_art_event_t;

typedef void (*bt_xstack_cover_art_callback_t)(
    const bt_xstack_cover_art_event_t *event, void *context);
```

---

### Task 1: Restore Native Cover Art Client and Feature Boundary

**Files:**
- Create: `btstack/src/classic/avrcp_cover_art_client.c`
- Create: `btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1`
- Modify: `btstack/include/btstack_config.h`
- Modify: `btstack/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`

- [ ] **Step 1: Write the failing upstream/build contract**

Create a PowerShell contract that asserts:

```powershell
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$source = Join-Path $root 'src/classic/avrcp_cover_art_client.c'
if (-not (Test-Path $source)) { throw 'missing native AVRCP Cover Art client source' }
$config = Get-Content -Raw (Join-Path $root 'include/btstack_config.h')
if ($config -notmatch '#define\s+BTSTACK_AVRCP_COVER_ART\s+1') { throw 'missing canonical Cover Art switch' }
if ($config -notmatch 'BTSTACK_CLASSIC.*BTSTACK_AVRCP_CONTROLLER.*BTSTACK_AVRCP_TARGET.*BTSTACK_AVRCP_COVER_ART') {
    throw 'native ENABLE_AVRCP_COVER_ART derivation lacks parent guards'
}
```

Also assert the API and AT projects each contain the native source once and the
Legacy project contains it zero times.

- [ ] **Step 2: Run the contract and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1
```

Expected: FAIL with `missing native AVRCP Cover Art client source`.

- [ ] **Step 3: Restore the exact official source and add the guarded feature**

Restore the contents from:

```text
https://raw.githubusercontent.com/bluekitchen/btstack/v1.6.2/src/classic/avrcp_cover_art_client.c
```

using `apply_patch`, preserving the copyright and upstream code. Add to the
canonical configuration:

```c
#ifndef BTSTACK_AVRCP_COVER_ART
#define BTSTACK_AVRCP_COVER_ART 1
#endif

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER && \
    BTSTACK_AVRCP_TARGET && BTSTACK_AVRCP_COVER_ART
#define ENABLE_AVRCP_COVER_ART
#endif
```

Add the native selected source group, not a common source:

```make
BTSTACK_COVER_ART_NATIVE_SOURCES := \
    $(BTSTACK_ROOT)/src/classic/avrcp_cover_art_client.c
```

The API and AT projects consume this group. Do not modify
`Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`.

- [ ] **Step 4: Run the upstream/build contract and existing feature matrix**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_feature_config.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_feature_config_disabled.ps1
```

Expected: all PASS; CT-only, TG-only, family-off, and Cover-Art-off builds do
not allocate or reference the native client.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/src/classic/avrcp_cover_art_client.c btstack/include/btstack_config.h btstack/Makefile.inc btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1 Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj
git commit -m "feat: restore native AVRCP cover art client"
```

---

### Task 2: Define Public and Internal Cover Art Contracts

**Files:**
- Create: `btstack/include/api/bt_xstack_cover_art.h`
- Create: `btstack/include/core/bt_xstack_cover_art.h`
- Create: `btstack/include/at/bt_xstack_cover_art.h`
- Create: `btstack/include/profiles/btstack_bip_client.h`
- Create: `btstack/include/profiles/btstack_avrcp_cover_art.h`
- Create: `btstack/api/tests/test_bt_xstack_cover_art_header.c`
- Create: `btstack/api/tests/test_bt_xstack_cover_art_header.cpp`
- Modify: `btstack/include/api/bt_xstack.h`
- Modify: `btstack/include/core/bt_xstack.h`
- Modify: `btstack/include/at/bt_xstack.h`

- [ ] **Step 1: Write C99 and C++11 header compile fixtures**

The C fixture must instantiate all public enums, property/descriptor variants,
all event-union arms, and both functions:

```c
static void on_cover_art(const bt_xstack_cover_art_event_t *event, void *context)
{
    (void)event;
    (void)context;
}

int main(void)
{
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG, 200u, 200u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_NONE
    };
    bt_xstack_cover_art_callback_t callback = on_cover_art;
    (void)descriptor;
    (void)callback;
    return 0;
}
```

Compile separate C++11 translation units for the API and AT include roots; each
assigns the callback without casts. Do not include both mutually exclusive
frontend headers in one translation unit.

- [ ] **Step 2: Compile fixtures and verify RED**

Run MinGW C99/C++11 with `-Wall -Wextra -Werror -fsyntax-only`.

Expected: FAIL because `bt_xstack_cover_art.h` does not exist.

- [ ] **Step 3: Add the public shapes and normalized internal event API**

Use the stable public shapes at the top of this plan. The API and core headers
add:

```c
bt_xstack_result_t bt_xstack_cover_art_set_mode(bt_xstack_cover_art_mode_t mode);
bt_xstack_result_t bt_xstack_cover_art_get_image(
    const bt_xstack_cover_art_image_descriptor_t *descriptor);
```

The AT header exposes the same event and descriptor types plus the callback
registration function added in Task 8. AT mode and image actions remain
textual commands.

The internal BIP header defines:

```c
typedef enum {
    BTSTACK_BIP_OPERATION_NONE = 0,
    BTSTACK_BIP_OPERATION_LINKED_THUMBNAIL,
    BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES,
    BTSTACK_BIP_OPERATION_IMAGE
} btstack_bip_operation_t;

int btstack_bip_client_init(btstack_bip_event_callback_t callback);
int btstack_bip_client_set_object(const btstack_bd_addr_t address,
    const char image_handle[8], uint32_t generation);
int btstack_bip_client_get_linked_thumbnail(void);
int btstack_bip_client_get_image_properties(void);
int btstack_bip_client_get_image(
    const bt_xstack_cover_art_image_descriptor_t *descriptor);
int btstack_bip_client_cancel(void);
void btstack_bip_client_peer_disconnected(const btstack_bd_addr_t address);
```

The coordinator header defines setup, mode, image request, native handle,
track-change, and disconnect entry points. All disabled implementations remain
linkable and return `BTSTACK_STATUS_NOT_SUPPORTED` internally.

- [ ] **Step 4: Re-run strict header compilation**

Expected: C99 and C++11 fixtures PASS without warnings; `sizeof` assertions
prove the property and descriptor contain no unbounded arrays.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/include/api/bt_xstack_cover_art.h btstack/include/core/bt_xstack_cover_art.h btstack/include/at/bt_xstack_cover_art.h btstack/include/profiles/btstack_bip_client.h btstack/include/profiles/btstack_avrcp_cover_art.h btstack/include/api/bt_xstack.h btstack/include/core/bt_xstack.h btstack/include/at/bt_xstack.h btstack/api/tests/test_bt_xstack_cover_art_header.c btstack/api/tests/test_bt_xstack_cover_art_header.cpp
git commit -m "feat: define cover art public contracts"
```

---

### Task 3: Implement Incremental BIP Image-Properties Parsing

**Files:**
- Create: `btstack/profiles/tests/test_btstack_bip_client.c`
- Create: `btstack/profiles/tests/test_btstack_bip_client.ps1`
- Create: `btstack/profiles/btstack_bip_client.c`
- Modify: `btstack/Makefile.inc`

- [ ] **Step 1: Write parser-first failing tests**

Feed XML in one-byte, split-attribute, and multi-entry chunks:

```xml
<image-properties version="1.0" handle="0123456">
  <native encoding="JPEG" pixel="640*640" size="182340" />
  <variant encoding="JPEG" pixel="200*200" transformation="stretch fill" />
  <variant encoding="PNG" pixel="100*100-300*300" />
</image-properties>
```

Assert three ordered property events with exact encoding, ranges, size-known,
native flag, and transformation bits. Add cases for malformed XML, unknown
encoding, `70000*1`, inverted range, `size=4294967296`, and missing required
encoding/pixel attributes.

- [ ] **Step 2: Run the parser test and verify RED**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_bip_client.ps1 -ParserOnly
```

Expected: compile/link FAIL because `btstack_bip_client.c` is absent.

- [ ] **Step 3: Implement fixed-state yxml parsing**

Use fixed fields in the client state:

```c
typedef struct {
    yxml_t parser;
    char parser_buffer[128];
    char element[24];
    char attribute[24];
    char value[64];
    uint8_t value_len;
    bt_xstack_cover_art_property_t property;
    uint8_t in_property;
    uint16_t property_count;
} btstack_bip_properties_parser_t;
```

Map only `native` and `variant`. Accept `JPEG`, `PNG`, `GIF`, `BMP`, and
`WBMP`; reject any other encoding with one terminal parse error.
Parse `W*H` and `W*H-W*H` with checked decimal conversion. Parse whitespace-
separated `stretch`, `fill`, and `crop`. Emit a property only on a complete
self-closing or closing entry. On malformed/overflow input, emit one terminal
parse error and reject subsequent bytes for that generation.

- [ ] **Step 4: Link yxml exactly once and run parser tests**

Move `3rd-party/yxml/yxml.c` into a shared XML dependency group consumed once
when PBAP or Cover Art is selected. Assert the API and AT projects contain one
yxml unit, never two. Add `profiles/btstack_bip_client.c` to a selected
`BTSTACK_BIP_SOURCES` group and to the API/AT projects only after this source
exists.

Expected: all parser cases PASS; no heap allocator symbol appears in the host
object.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/profiles/btstack_bip_client.c btstack/profiles/tests/test_btstack_bip_client.c btstack/profiles/tests/test_btstack_bip_client.ps1 btstack/Makefile.inc
git commit -m "feat: parse BIP image properties incrementally"
```

---

### Task 4: Complete the Single-Session BIP State Machine

**Files:**
- Modify: `btstack/profiles/btstack_bip_client.c`
- Modify: `btstack/profiles/tests/test_btstack_bip_client.c`

- [ ] **Step 1: Add failing native-session behavior tests**

Stub the official client entry points and capture its packet handler. Cover:

- object set copies address/handle/generation;
- connect success then linked-thumbnail GET;
- properties data parsed before operation-complete;
- Image Descriptor serialization for JPEG 320x240 crop;
- BIP data offsets and final byte count;
- second operation returns `BTSTACK_STATUS_BUSY`;
- native connect/GET/disconnect immediate failure returns to IDLE;
- synchronous connection/operation callbacks do not overwrite terminal state;
- peer disconnect, explicit cancel, and stale CID do not double-complete.

Use this expected descriptor serialization:

```xml
<image-descriptor version="1.0"><image encoding="JPEG" pixel="320*240" transformation="crop" /></image-descriptor>
```

- [ ] **Step 2: Run the full BIP client test and verify RED**

Expected: parser cases pass; session cases fail at the first missing native
connect call.

- [ ] **Step 3: Implement one static session and bounded descriptor builder**

Use explicit states:

```c
typedef enum {
    BTSTACK_BIP_STATE_IDLE = 0,
    BTSTACK_BIP_STATE_CONNECTING,
    BTSTACK_BIP_STATE_CONNECTED,
    BTSTACK_BIP_STATE_GETTING_PROPERTIES,
    BTSTACK_BIP_STATE_GETTING_IMAGE,
    BTSTACK_BIP_STATE_GETTING_THUMBNAIL,
    BTSTACK_BIP_STATE_DISCONNECTING
} btstack_bip_state_t;
```

Allocate one `avrcp_cover_art_client_t`, one 2000-byte ERTM buffer, one fixed
ERTM config, one eight-byte handle, and one 160-byte descriptor buffer behind
the full feature guard. Native `BIP_DATA_PACKET` routes to the parser only for
properties; image/thumbnail data emits transient chunks. Increment offset only
after the normalized callback returns. Terminal paths snapshot event data,
reset state, then invoke callbacks so callback reentry can start a new
generation safely.

- [ ] **Step 4: Run BIP client tests repeatedly**

```powershell
1..20 | ForEach-Object {
  powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_bip_client.ps1
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: 20/20 PASS with no stale/busy/synchronous-callback failure.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/profiles/btstack_bip_client.c btstack/profiles/tests/test_btstack_bip_client.c
git commit -m "feat: add serialized BIP image operations"
```

---

### Task 5: Implement Automatic AVRCP Cover Art Policy

**Files:**
- Create: `btstack/profiles/btstack_avrcp_cover_art.c`
- Create: `btstack/profiles/tests/test_btstack_avrcp_cover_art.c`
- Create: `btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1`
- Modify: `btstack/profiles/btstack_avrcp_controller.c`
- Modify: `btstack/profiles/btstack_avrcp.c`

- [ ] **Step 1: Write failing policy tests**

Exercise:

```c
btstack_avrcp_cover_art_connected(peer);
btstack_avrcp_cover_art_handle_received((const uint8_t *)"0123456", 7u);
```

In thumbnail mode assert object set plus one linked-thumbnail request. In image
mode assert object set plus one properties request and no image request before
the application descriptor. Add invalid handle lengths 0, 6, and 8; duplicate
same-handle notification; track change during active download; mode change;
disconnect; newest-handle-wins pending replacement; generation wrap from
`UINT32_MAX` to 1; and callback reentry.

- [ ] **Step 2: Run policy test and verify RED**

Expected: FAIL because the coordinator functions are undefined.

- [ ] **Step 3: Implement the coordinator**

Use one fixed policy state:

```c
typedef struct {
    btstack_bd_addr_t peer;
    char handle[8];
    char pending_handle[8];
    uint32_t generation;
    bt_xstack_cover_art_mode_t mode;
    uint8_t connected;
    uint8_t operation_active;
    uint8_t pending_valid;
    uint8_t retry_used;
} btstack_avrcp_cover_art_context_t;
```

On native handle reception validate seven non-NUL bytes, append NUL locally,
advance generation, and start the selected operation when a callback is
registered. On track/mode change cancel once; retain only the latest pending
handle. Retry one terminal connect/OBEX failure only after cleanup, never from
inside a synchronous failed dispatch frame.

Add the native controller case:

```c
case AVRCP_SUBEVENT_NOW_PLAYING_COVER_ART_INFO:
    length = avrcp_subevent_now_playing_cover_art_info_get_value_len(packet);
    value = avrcp_subevent_now_playing_cover_art_info_get_value(packet);
    btstack_avrcp_cover_art_handle_received(value, length);
    return;
```

- [ ] **Step 4: Run AVRCP policy and existing metadata tests**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_avrcp_default_policy.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/ports/gpcv2178/tests/test_avrcp_metadata_response_completion.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/ports/gpcv2178/tests/test_avrcp_parser_reset_value_offset.ps1
```

Expected: all PASS; existing title/artist/album behavior remains unchanged.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/profiles/btstack_avrcp_cover_art.c btstack/profiles/btstack_avrcp_controller.c btstack/profiles/btstack_avrcp.c btstack/profiles/tests/test_btstack_avrcp_cover_art.c btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1
git commit -m "feat: automate current-track cover art retrieval"
```

---

### Task 6: Route Cover Art Through Profile Manager and Event Mapper

**Files:**
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/core/btstack_event_mapper.c`
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/core/btstack_core_dispatcher.c`
- Modify: `btstack/core/tests/test_btstack_profile_manager.c`
- Modify: `btstack/core/tests/test_btstack_profile_manager.ps1`

- [ ] **Step 1: Add failing manager/mapper tests**

Assert initialization order after AVRCP setup, one normalized subscription,
all six public event types, transient DATA pointer preservation, callback
outside critical section, mode/image dispatch on Stack Run Loop, disabled
feature behavior, and no Cover Art event in `btstack_core_event_envelope_t`.

- [ ] **Step 2: Run manager test and verify RED**

Expected: FAIL at missing BIP/Cover Art initialization and missing direct
delivery function.

- [ ] **Step 3: Add request and direct-delivery boundaries**

Extend the request enum and union:

```c
BT_XSTACK_REQUEST_COVER_ART_SET_MODE,
BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE,

struct {
    bt_xstack_cover_art_mode_t mode;
    bt_xstack_cover_art_image_descriptor_t descriptor;
} cover_art;
```

Add:

```c
void btstack_core_event_mapper_cover_art(const btstack_bip_event_t *event);
void btstack_frontend_deliver_cover_art(
    const bt_xstack_cover_art_event_t *event);
```

Profile Manager initializes the BIP client, initializes the automatic
coordinator, and gives the coordinator's normalized callback to Event Mapper.
Dispatcher calls coordinator mode/image functions. Mapper constructs one
stack-local public event and invokes the frontend delivery function directly.
It never calls `btstack_core_post_event()` for Cover Art.

- [ ] **Step 4: Run manager, event, ring, and boundary tests**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_profile_manager.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/api/tests/test_bt_xstack_event_dispatch_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_ring.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_platform_boundary.ps1
```

Expected: all PASS; queue capacities remain API 8/16/16 with reserve 4 and AT
8/1/16.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/core/btstack_profile_manager.c btstack/core/btstack_event_mapper.c btstack/core/btstack_core_internal.h btstack/core/btstack_core_dispatcher.c btstack/core/tests/test_btstack_profile_manager.c btstack/core/tests/test_btstack_profile_manager.ps1
git commit -m "feat: route normalized cover art events"
```

---

### Task 7: Implement API Admission and Direct Callback Delivery

**Files:**
- Create: `btstack/api/tests/test_bt_xstack_cover_art.c`
- Create: `btstack/api/tests/test_bt_xstack_cover_art.ps1`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Modify: `btstack/include/api/bt_xstack.h`
- Modify: `btstack/include/core/bt_xstack.h`

- [ ] **Step 1: Write the failing real-core API test**

Compile the real `bt_xstack_api_core.c` with fixed-ring/worker stubs. After
successful init assert:

- invalid mode and invalid descriptor return `BT_XSTACK_INVALID_ARGUMENT` and
  submit zero requests;
- valid mode and image descriptor submit one deeply copied request;
- queue full returns `BT_XSTACK_QUEUE_FULL`;
- direct STARTED/DATA/COMPLETED callbacks receive the configured context;
- DATA pointer and length are unchanged during callback;
- callback is invoked with platform critical depth zero;
- NULL callback suppresses automatic retrieval via the manager callback-
  presence boundary.

- [ ] **Step 2: Run API test and verify RED**

Expected: link FAIL for missing public functions and delivery boundary.

- [ ] **Step 3: Implement API functions and callback snapshot**

Add `cover_art_callback` to `bt_xstack_config_t`. Validate before request
submission:

```c
if (mode > BT_XSTACK_COVER_ART_MODE_IMAGE) return BT_XSTACK_INVALID_ARGUMENT;
if ((descriptor == NULL) ||
    (descriptor->encoding == BT_XSTACK_COVER_ART_ENCODING_UNKNOWN) ||
    (descriptor->width == 0u) || (descriptor->height == 0u) ||
    ((descriptor->transformation & ~BT_XSTACK_COVER_ART_TRANSFORMATION_ALL) != 0u)) {
    return BT_XSTACK_INVALID_ARGUMENT;
}
```

Add `profiles/btstack_avrcp_cover_art.c` to a selected
`BTSTACK_AVRCP_COVER_ART_POLICY_SOURCES` group and to the API/AT projects only
after the coordinator source exists.

Copy the descriptor into `btstack_core_request_t`. In
`btstack_frontend_deliver_cover_art`, snapshot callback/context under platform
critical, leave critical, then invoke the callback.

- [ ] **Step 4: Run API Cover Art and queue tests**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/api/tests/test_bt_xstack_cover_art.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/api/tests/test_bt_xstack_api_queue_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/api/tests/test_bt_xstack_api_backend_contract.ps1
```

Expected: all PASS; no request stores an application pointer.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/api/bt_xstack_api_core.c btstack/include/api/bt_xstack.h btstack/include/core/bt_xstack.h btstack/api/tests/test_bt_xstack_cover_art.c btstack/api/tests/test_bt_xstack_cover_art.ps1
git commit -m "feat: expose API cover art callbacks"
```

---

### Task 8: Implement Public AT Mode/Image Commands and Binary Callback

**Files:**
- Create: `btstack/at/tests/test_btstack_at_cover_art.c`
- Create: `btstack/at/tests/test_btstack_at_cover_art.ps1`
- Modify: `btstack/include/at/bt_xstack.h`
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: `btstack/at/btstack_at_command_table.c`
- Modify: `btstack/at/btstack_at_dispatch_table.c`
- Modify: `btstack/at/btstack_at_parser.c`
- Modify: `btstack/at/btstack_at_core_adapter.c`

- [ ] **Step 1: Write failing AT parser/dispatch/callback tests**

Use these exact commands:

```text
AT+COVERMODE=0
AT+COVERMODE=1
AT+COVERIMAGE=JPEG,320,240,NONE
AT+COVERIMAGE=PNG,200,200,CROP
```

Reject missing tokens, unknown encodings/transforms, zero dimensions, values
above 65535, and trailing tokens. Assert dispatch rejects `COVERIMAGE` while
mode is thumbnail. Accepted commands deep-copy the descriptor into one core request. Register a
binary callback and assert raw bytes are not formatted, copied into the AT
Event Ring, or delivered through the textual output callback.

- [ ] **Step 2: Run AT Cover Art test and verify RED**

Expected: parser FAIL at unknown `COVERMODE` command.

- [ ] **Step 3: Add bounded AT syntax and binary delivery**

Add:

```c
int bt_xstack_cover_art_register_callback(
    bt_xstack_cover_art_callback_t callback, void *context);
```

Store callback/context under platform critical. Parse encoding and
transformation by exact case-insensitive token match into enums; store no raw
token pointer. Route commands through the existing AT request admission path.
Implement `btstack_frontend_deliver_cover_art()` as a callback/context snapshot
followed by a lock-free invocation. Keep Cover Art events out of
`btstack_at_event_t` and `btstack_at_post_event()`.

- [ ] **Step 4: Run all AT focused tests**

```powershell
Get-ChildItem btstack/at/tests/*.ps1 | ForEach-Object {
  powershell -NoProfile -ExecutionPolicy Bypass -File $_.FullName
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: every AT test PASS; Input/Completion/Event Ring capacities remain
8/1/16 and no 20 ms polling returns.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/include/at/bt_xstack.h btstack/at/btstack_at_internal.h btstack/at/btstack_at_command_table.c btstack/at/btstack_at_dispatch_table.c btstack/at/btstack_at_parser.c btstack/at/btstack_at_core_adapter.c btstack/at/tests/test_btstack_at_cover_art.c btstack/at/tests/test_btstack_at_cover_art.ps1
git commit -m "feat: add AT cover art control and binary callback"
```

---

### Task 9: Harden Cancellation, Backpressure, and Reentry

**Files:**
- Modify: `btstack/profiles/tests/test_btstack_bip_client.c`
- Modify: `btstack/profiles/tests/test_btstack_avrcp_cover_art.c`
- Modify: `btstack/profiles/btstack_bip_client.c`
- Modify: `btstack/profiles/btstack_avrcp_cover_art.c`
- Create: `btstack/core/tests/test_btstack_cover_art_build_contract.ps1`

- [ ] **Step 1: Add mutation-resistant terminal-state tests**

Add explicit callback sequence numbers and assert:

- STARTED precedes DATA;
- all DATA offsets are contiguous;
- exactly one COMPLETED or ERROR follows each STARTED;
- state is IDLE before terminal callback reentry;
- track change during connect/data/disconnect selects only newest generation;
- native synchronous close cannot overwrite a new callback-started operation;
- immediate native disconnect failure cannot leave BUSY;
- duplicate handle does not redownload;
- callback unregister during DATA suppresses later public DATA but cleanup still
  completes;
- one retry occurs only after a cleanup edge, and second failure is terminal.

Add source mutation checks that fail if generation guards, descriptor bounds,
or direct-callback lock exit are removed.

- [ ] **Step 2: Run tests against one deliberate mutant and verify RED**

In a temporary copy only, remove the generation comparison before DATA emit.
Run both behavior scripts.

Expected: FAIL on stale DATA delivery. Delete the temporary mutant; do not edit
the production source for this check.

- [ ] **Step 3: Implement the minimal guards exposed by RED**

The BIP terminal helper must follow this order:

```c
saved_event = context->terminal_event;
btstack_bip_reset_to_idle();
btstack_bip_emit(&saved_event);
```

Every native callback first validates current CID, operation, and generation.
The coordinator clears active state and schedules the newest pending generation
before forwarding the terminal event to Profile Manager; it performs no global
write after the public callback can run. Immediate
disconnect/GOEP failure emits the original terminal status after reset.

- [ ] **Step 4: Run BIP/Cover Art tests 50 times and contracts once**

```powershell
1..50 | ForEach-Object {
  powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_bip_client.ps1
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  powershell -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/core/tests/test_btstack_cover_art_build_contract.ps1
```

Expected: 100 behavior runs and the build contract PASS.

- [ ] **Step 5: Checkpoint commit if Git exists**

```bash
git add btstack/profiles/btstack_bip_client.c btstack/profiles/btstack_avrcp_cover_art.c btstack/profiles/tests/test_btstack_bip_client.c btstack/profiles/tests/test_btstack_avrcp_cover_art.c btstack/core/tests/test_btstack_cover_art_build_contract.ps1
git commit -m "fix: harden cover art cancellation and reentry"
```

---

### Task 10: Regenerate Only API/AT Builds and Verify Artifacts

**Files:**
- Modify: generated `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library_Debug.mk`
- Modify: generated `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library_Debug.mk`
- Modify: packaged API and AT public headers produced by official builds
- Do not modify: `Project/WirelessLibraries/BTstack_Library/**`

- [ ] **Step 1: Run all new and adjacent host tests**

```powershell
$tests = @(
  'btstack/profiles/tests/test_btstack_cover_art_upstream_contract.ps1',
  'btstack/profiles/tests/test_btstack_bip_client.ps1',
  'btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1',
  'btstack/api/tests/test_bt_xstack_cover_art.ps1',
  'btstack/at/tests/test_btstack_at_cover_art.ps1',
  'btstack/core/tests/test_btstack_profile_manager.ps1',
  'btstack/api/tests/test_bt_xstack_event_dispatch_contract.ps1',
  'btstack/core/tests/test_btstack_platform_boundary.ps1',
  'btstack/core/tests/test_btstack_selected_build_variants.ps1'
)
foreach ($test in $tests) {
  powershell -NoProfile -ExecutionPolicy Bypass -File $test
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: 9/9 PASS.

- [ ] **Step 2: Regenerate and force-build API and AT only**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1 -Jobs 4
powershell -NoProfile -ExecutionPolicy Bypass -File Project/WirelessLibraries/BTstack_AT_Library/build_btstack_at.ps1 -Jobs 4
```

Expected: both exit 0. Only the pre-existing upstream AVDTP
`uuid maybe-uninitialized` and AVRCP Controller `register_status unused`
warnings are permitted; Cover Art/BIP warnings are zero.

- [ ] **Step 3: Verify exact archive membership and Legacy non-modification**

Using the project ARM `arm-none-eabi-ar` and `arm-none-eabi-nm`, assert each API
and AT archive contains exactly one of:

```text
avrcp_cover_art_client.o
btstack_bip_client.o
btstack_avrcp_cover_art.o
```

Assert one public `bt_xstack_cover_art_set_mode`, one public
`bt_xstack_cover_art_get_image`, one native
`avrcp_cover_art_client_get_linked_thumbnail`, and no duplicate yxml object.
Capture the Legacy `.gpcmprj`, generated makefile, and archive SHA/mtime before
Task 10 and assert they are unchanged afterward.

- [ ] **Step 4: Verify memory and package boundaries**

Use `arm-none-eabi-nm -S --size-sort` to record the ERTM buffer, BIP state,
descriptor buffer, and parser state. Assert:

- no symbol resembles a complete-image buffer;
- no new worker stack/task/queue symbol;
- no `malloc`, `calloc`, `realloc`, or `free` reference from the three new
  objects;
- API and AT packaged Cover Art headers SHA-match their source headers;
- binary DATA does not occur in `btstack_core_event_envelope_t` or
  `btstack_at_event_t`.

- [ ] **Step 5: Run final relevant test enumeration**

Run every `*.ps1` under `btstack/api/tests`, `btstack/profiles/tests`,
`btstack/core/tests`, and `btstack/at/tests` in a fresh PowerShell process.
Report every failure by name; do not hide the known unrelated missing-patch
baseline if it remains. Re-run the new Cover Art tests after the official
builds so staged headers and archive freshness are covered.

- [ ] **Step 6: Checkpoint commit if Git exists**

```bash
git add btstack Project/WirelessLibraries/BTstack_API_Library Project/WirelessLibraries/BTstack_AT_Library docs/superpowers/specs/2026-08-20-avrcp-cover-art-bip-client-design.md docs/superpowers/plans/2026-08-20-avrcp-cover-art-bip-client.md
git commit -m "feat: add AVRCP cover art and BIP image retrieval"
```

Do not add any path under
`Project/WirelessLibraries/BTstack_Library`.
