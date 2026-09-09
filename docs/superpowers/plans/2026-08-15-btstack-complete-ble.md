# BTstack Complete BLE Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver working Legacy BLE Central and Peripheral support, bounded dynamic GATT Server, GATT Client, HOGP, and matching API/AT frontends on the existing worker/ring architecture.

**Architecture:** Consolidate GAP into common, Classic, LE, and AD-data units. Keep native BLE parsing inside GAP or the owning GATT/Profile module, let Profile Manager own lifecycle and cross-module state, and keep Core Dispatcher/Event Mapper thin. API and AT remain mutually exclusive frontends over the same fixed-capacity requests and events.

**Tech Stack:** C99 embedded C, BlueKitchen BTstack 1.6.2 ATT/GATT/SM, fixed `btstack_ring`, PowerShell host contracts, MinGW host fixtures, ARM GCC/G+ generated library projects.

---

## Execution rules

- Follow TDD for every behavior change: add a failing behavior/compile contract, observe RED, implement the smallest complete behavior, then rerun GREEN.
- Do not add a BLE task, dynamic queue, polling loop, weak registry, or second `btstack_config.h`.
- Do not call application callbacks from the BTstack Run Loop.
- Preserve the existing API/AT build-variant split.
- This workspace has no Git metadata. Replace each commit step with a recorded changed-file list and fresh test output; do not fabricate commits.
- The known missing `patches/wireless_static_library_integration.patch` failure is not a pass condition for this plan, but all other focused and full contracts must pass.

## File structure

### GAP

- Modify: `btstack/gap/btstack_gap.c` — common state, public facade, one native HCI registration.
- Create: `btstack/gap/btstack_gap_classic.c` — existing Classic link, inquiry, and pairing behavior.
- Create: `btstack/gap/btstack_gap_le.c` — advertising, scanning, single LE link, SM, Bond, and privacy.
- Keep: `btstack/gap/btstack_gap_ad_data.c` — stateless AD builder.
- Modify: `btstack/include/gap/btstack_gap.h` — consolidated Classic and LE contract.
- Keep: `btstack/include/gap/btstack_gap_ad_data.h`.
- Delete after migration: `btstack/gap/btstack_gap_adv.c`, `btstack_gap_link.c`, `btstack_gap_privacy.c`, `btstack_gap_scan.c`, `btstack_gap_security.c` and their matching headers.

### Profiles

- Create: `btstack/profiles/btstack_gatt_server.c` and `btstack/include/profiles/btstack_gatt_server.h`.
- Create: `btstack/profiles/btstack_gatt_client.c` and `btstack/include/profiles/btstack_gatt_client.h`.
- Delete after migration: `btstack/profiles/btstack_gatt.c` and `btstack/include/profiles/btstack_gatt.h`.
- Modify: `btstack/profiles/btstack_hogp_device.c` and its header.
- Modify: `btstack/core/btstack_profile_manager.c` and its header.

### Core and public frontends

- Modify: `btstack/core/btstack_core_internal.h`, `btstack/core/btstack_core_dispatcher.c`, `btstack/core/btstack_event_mapper.c`.
- Create source headers: `btstack/include/core/bt_xstack_le.h`, `btstack/include/core/bt_xstack_gatt.h`.
- Create packaged headers: `btstack/include/api/bt_xstack_le.h`, `btstack/include/api/bt_xstack_gatt.h` through the existing package/build flow.
- Modify both source and packaged `bt_xstack.h`, `bt_xstack_types.h`, and affected service headers.
- Modify: `btstack/api/bt_xstack_api_core.c`.
- Modify AT parser/protocol/table/adapter files under `btstack/at/`.

### Build and tests

- Modify: `btstack/include/btstack_config.h`, `btstack/Makefile.inc`.
- Modify the API, AT, and Legacy `.gpcmprj` files and regenerate their Debug makefiles.
- Add focused tests under `btstack/gap/tests`, `btstack/profiles/tests`, `btstack/core/tests`, `btstack/api/tests`, and `btstack/at/tests`.

---

### Task 1: Lock the BLE configuration and upstream source boundary

**Files:**
- Modify: `btstack/include/btstack_config.h`
- Modify: `btstack/Makefile.inc`
- Test: `btstack/core/tests/test_btstack_ble_build_contract.ps1`
- Test: `btstack/core/tests/test_btstack_feature_config.ps1`
- Test: `btstack/core/tests/test_btstack_feature_config_disabled.ps1`

- [ ] **Step 1: Write the failing configuration/build contract**

Create a contract that requires these exact defaults and dependency checks:

```c
#define BTSTACK_LE_CONNECTION_MAX       1u
#define BTSTACK_GATT_SERVICE_MAX        4u
#define BTSTACK_GATT_CHARACTERISTIC_MAX 8u
#define BTSTACK_GATT_DESCRIPTOR_MAX     8u
#define BTSTACK_GATT_DB_MAX          1024u
#define BTSTACK_GATT_VALUE_POOL_MAX  1024u
#define BTSTACK_GATT_VALUE_MAX        244u
#define BTSTACK_LE_BOND_MAX             4u

#if !BTSTACK_LE && (BTSTACK_GATT_CLIENT || BTSTACK_GATT_SERVER || BTSTACK_HOGP_DEVICE)
#error "LE profiles require BTSTACK_LE"
#endif
#if BTSTACK_HOGP_DEVICE && !BTSTACK_GATT_SERVER
#error "BTSTACK_HOGP_DEVICE requires BTSTACK_GATT_SERVER"
#endif
```

The contract must also require the selected common source set to contain exactly once:

```text
src/ble/att_db.c
src/ble/att_dispatch.c
src/ble/att_server.c
src/ble/gatt_client.c
src/ble/sm.c
src/ble/le_device_db_memory.c
src/btstack_crypto.c
```

The current tree does not contain `src/ble/gatt_client.c`. Restore that one file from the official BTstack v1.6.2 tag, record its raw SHA-256 in the contract, and reject any other upstream version. Do not reconstruct the client implementation locally from the header.

- [ ] **Step 2: Run the new contract and observe RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_ble_build_contract.ps1
```

Expected: failure naming missing BLE capacities, dependency checks, or upstream source units.

- [ ] **Step 3: Add the config constants, compile-time checks, and common BLE source group**

Add a `BTSTACK_BLE_CORE_SOURCES` group to `btstack/Makefile.inc`, and include it in `BTSTACK_COMMON_SOURCES` only when the selected build enables `BTSTACK_LE=1`. Keep GATT Client and ATT Server source selection separately checkable even if the initial project enables both.

Extend the internal status enum with stable terminal reasons used by BLE operations:

```c
BTSTACK_STATUS_SECURITY_ERROR = -9,
BTSTACK_STATUS_DISCONNECTED = -10
```

- [ ] **Step 4: Prove the configuration matrix**

Compile minimal fixtures for these exact combinations:

```text
LE=0 CLIENT=0 SERVER=0 HOGP=0 -> pass
LE=0 CLIENT=1 SERVER=0 HOGP=0 -> fail
LE=1 CLIENT=1 SERVER=0 HOGP=0 -> pass
LE=1 CLIENT=0 SERVER=1 HOGP=0 -> pass
LE=1 CLIENT=0 SERVER=0 HOGP=1 -> fail
LE=1 CLIENT=0 SERVER=1 HOGP=1 -> pass
```

Run the new contract plus both existing feature-config scripts. Expected: all pass.

- [ ] **Step 5: Record Task 1 evidence**

Record the changed config/build files and the six compile-matrix results in the plan execution log.

---

### Task 2: Consolidate the existing GAP implementation without changing Classic behavior

**Files:**
- Modify: `btstack/gap/btstack_gap.c`
- Create: `btstack/gap/btstack_gap_classic.c`
- Modify: `btstack/include/gap/btstack_gap.h`
- Modify: `btstack/core/btstack_adapter.c`
- Delete after GREEN: old link/scan/security/adv/privacy source and headers listed in File Structure
- Test: `btstack/gap/tests/test_btstack_gap_classic.c`
- Test: `btstack/gap/tests/test_btstack_gap_contract.ps1`
- Test: `btstack/core/tests/test_btstack_adapter.c`

- [ ] **Step 1: Extend tests to require one GAP init and one HCI registration**

The behavior fixture must call:

```c
assert(btstack_gap_init() == BTSTACK_STATUS_OK);
assert(btstack_gap_init() == BTSTACK_STATUS_OK);
assert(test_hci_registration_count == 1u);
assert(btstack_gap_classic_scan_start(5u) == BTSTACK_STATUS_OK);
assert(btstack_gap_classic_scan_stop() == BTSTACK_STATUS_OK);
```

The static contract must require only the four approved GAP `.c` files and forbid the five deleted filenames.

- [ ] **Step 2: Run the focused GAP and Adapter tests and observe RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/gap/tests/test_btstack_gap_contract.ps1
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_adapter.ps1
```

Expected: failure because Classic operations still live in the old units and Adapter still invokes separate adv/privacy init functions.

- [ ] **Step 3: Move Classic code and simplify Adapter**

Expose internal Classic helpers only to `btstack_gap.c`:

```c
int btstack_gap_classic_init(void);
void btstack_gap_classic_reset(void);
int btstack_gap_classic_handle_hci_event(const uint8_t *packet, uint16_t size);
```

Move link, inquiry, PIN, SSP, and confirmation code without changing status mapping. Change `btstack_adapter_setup()` to call only `btstack_gap_init()` and remove `btstack_gap_adv_init()` / `btstack_gap_privacy_init()` calls.

- [ ] **Step 4: Run Classic regression tests before deleting old files**

Run GAP, Adapter, dispatcher, API backend, AT backend, and Classic profile tests. Expected: all pass with the old units excluded from compilation.

- [ ] **Step 5: Delete obsolete GAP units and rerun source-layout contracts**

Delete only the migrated source/header files. Run `rg` to prove there are no production includes or calls to their old symbols. Expected: zero matches outside migration-history documents.

---

### Task 3: Implement Legacy BLE advertising, scanning, and single-link GAP

**Files:**
- Create: `btstack/gap/btstack_gap_le.c`
- Modify: `btstack/gap/btstack_gap.c`
- Modify: `btstack/include/gap/btstack_gap.h`
- Test: `btstack/gap/tests/test_btstack_gap_le.c`
- Test: `btstack/gap/tests/test_btstack_gap_le.ps1`

- [ ] **Step 1: Define the bounded LE GAP types in the test first**

Use these concrete shapes in the production header and fixture:

```c
typedef enum {
    BTSTACK_GAP_LE_ROLE_NONE = 0,
    BTSTACK_GAP_LE_ROLE_CENTRAL,
    BTSTACK_GAP_LE_ROLE_PERIPHERAL
} btstack_gap_le_role_t;

typedef struct {
    uint16_t interval_min;
    uint16_t interval_max;
    uint8_t advertising_type;
    uint8_t own_address_type;
    uint8_t channel_map;
    uint8_t filter_policy;
    uint8_t data_len;
    uint8_t data[BTSTACK_ADV_DATA_MAX];
    uint8_t scan_response_len;
    uint8_t scan_response[BTSTACK_ADV_DATA_MAX];
} btstack_gap_le_adv_config_t;

typedef struct {
    uint16_t interval;
    uint16_t window;
    uint8_t active;
    uint8_t duplicate_filter;
} btstack_gap_le_scan_config_t;
```

- [ ] **Step 2: Write RED behavior cases**

Cover exact 31-byte acceptance, 32-byte rejection, start-before-working `NOT_READY`, advertising/scanning/connecting mutual exclusion, scan report deep copy, one Central connect, Peripheral connection complete, second link `BUSY`, cancel, disconnect, and cleanup.

- [ ] **Step 3: Run the LE GAP fixture and observe RED**

Run `test_btstack_gap_le.ps1`. Expected: compile failure because LE GAP functions/events do not exist.

- [ ] **Step 4: Implement the minimal complete LE state machine**

Implement these operations over upstream GAP:

```c
int btstack_gap_le_adv_configure(const btstack_gap_le_adv_config_t *config);
int btstack_gap_le_adv_start(void);
int btstack_gap_le_adv_stop(void);
int btstack_gap_le_scan_start(const btstack_gap_le_scan_config_t *config);
int btstack_gap_le_scan_stop(void);
int btstack_gap_le_connect(uint8_t address_type, const btstack_bd_addr_t address);
int btstack_gap_le_connect_cancel(void);
int btstack_gap_le_disconnect(void);
```

Handle `GAP_EVENT_ADVERTISING_REPORT`, `HCI_SUBEVENT_LE_CONNECTION_COMPLETE`, enhanced connection complete if emitted, connection update complete, and HCI disconnection complete. Emit normalized GAP events only after state is updated.

- [ ] **Step 5: Run GAP LE and existing Classic GAP tests**

Expected: both pass; there is still exactly one GAP HCI registration.

---

### Task 4: Implement SM, RAM Bonding, and privacy

**Files:**
- Modify: `btstack/gap/btstack_gap_le.c`
- Modify: `btstack/include/gap/btstack_gap.h`
- Test: `btstack/gap/tests/test_btstack_gap_le_security.c`
- Test: `btstack/gap/tests/test_btstack_gap_le_security.ps1`

- [ ] **Step 1: Add RED security scenarios**

Cover:

```c
assert(btstack_gap_le_security_init() == BTSTACK_STATUS_OK);
assert(test_sm_init_count == 1u);
assert(test_io_capability == IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
assert(test_auth_req & SM_AUTHREQ_BONDING);
assert(btstack_gap_le_pair() == BTSTACK_STATUS_NOT_READY);
assert(btstack_gap_le_pairing_confirm(0x0040u, true) == BTSTACK_STATUS_OK);
assert(btstack_gap_le_passkey_reply(0x0040u, 123456u) == BTSTACK_STATUS_OK);
```

Also cover RAM Bond capacity 4, delete-one, delete-all, identity resolving success/failure, random-address update, stack reset, and pairing completion. A process restart must begin with an empty Device DB.

- [ ] **Step 2: Observe RED**

Run the security fixture. Expected: missing functions and missing SM event mapping.

- [ ] **Step 3: Bind SM and storage**

Initialize `sm_init()`, register one SM handler, configure Just Works + Bonding, set the privacy address mode, and use `le_device_db_memory.c` for four current-boot bond records. Do not silently evict a fifth record and do not call the GPCV2178 storage stub.

- [ ] **Step 4: Implement user responses and deletion**

Map Just Works, numeric comparison, passkey display/input, pairing complete, reencryption, and identity resolution into GAP LE events. Validate connection handle and passkey range before calling upstream response functions.

- [ ] **Step 5: Run security, platform storage, GAP, and disabled-config tests**

Expected: all pass, `BTSTACK_LE=0` compiles without SM references, and the selected BLE object set has no persistent-storage dependency.

---

### Task 5: Replace the GATT Server skeleton with a bounded ATT database

**Files:**
- Create: `btstack/profiles/btstack_gatt_server.c`
- Create: `btstack/include/profiles/btstack_gatt_server.h`
- Delete after GREEN: `btstack/profiles/btstack_gatt.c`, `btstack/include/profiles/btstack_gatt.h`
- Test: `btstack/profiles/tests/test_btstack_gatt_server.c`
- Test: `btstack/profiles/tests/test_btstack_gatt_server.ps1`

- [ ] **Step 1: Define exact server registration types and RED tests**

```c
typedef enum {
    BTSTACK_GATT_SECURITY_NONE = 0,
    BTSTACK_GATT_SECURITY_ENCRYPTED,
    BTSTACK_GATT_SECURITY_AUTHENTICATED
} btstack_gatt_security_t;

typedef struct {
    btstack_uuid_t uuid;
    uint16_t properties;
    uint16_t max_len;
    btstack_gatt_security_t read_security;
    btstack_gatt_security_t write_security;
} btstack_gatt_server_characteristic_config_t;
```

Test four services accepted/fifth rejected, eight characteristics accepted/ninth rejected, eight descriptors accepted/ninth rejected, 1024-byte DB/value-pool exact boundary, build freeze, set/get, read, write, CCCD, Notify, Indicate, and indication completion.

- [ ] **Step 2: Run RED**

Expected: compile failure because the split GATT Server API is absent.

- [ ] **Step 3: Implement fixed tables and DB encoder**

Use static arrays sized by config and a fixed DB byte buffer. Encode service, characteristic declaration/value, descriptor, and CCCD records directly in BTstack ATT DB format. Return `NO_MEMORY` before mutating state when any table, DB, or value-pool limit would be exceeded.

- [ ] **Step 4: Bind ATT Server callbacks**

Initialize `att_server_init(database, read_callback, write_callback)`. Reads return the cached bounded value. Writes validate offset, total length, security, and max length, then copy and emit one peer-write event. Notify/Indicate validate link, property, CCCD, and payload length before using ATT Server send APIs.

- [ ] **Step 5: Run server fixture and memory mutation guards**

Mutation checks must fail if build freeze is removed, a capacity check is changed to overwrite, or a peer write invokes the application callback directly.

---

### Task 6: Implement serialized GATT Client operations

**Files:**
- Create: `btstack/profiles/btstack_gatt_client.c`
- Create: `btstack/include/profiles/btstack_gatt_client.h`
- Test: `btstack/profiles/tests/test_btstack_gatt_client.c`
- Test: `btstack/profiles/tests/test_btstack_gatt_client.ps1`

- [ ] **Step 1: Define operation and event types in the failing fixture**

```c
typedef enum {
    BTSTACK_GATT_CLIENT_OPERATION_NONE = 0,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_SERVICES,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_CHARACTERISTICS,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_DESCRIPTORS,
    BTSTACK_GATT_CLIENT_OPERATION_READ,
    BTSTACK_GATT_CLIENT_OPERATION_WRITE,
    BTSTACK_GATT_CLIENT_OPERATION_SUBSCRIBE,
    BTSTACK_GATT_CLIENT_OPERATION_UNSUBSCRIBE
} btstack_gatt_client_operation_t;
```

Test UUID16/UUID128 discovery, read value, Write Request, Write Without Response, descriptor discovery, CCCD subscription, notification, indication, query complete, BUSY, disconnect cancellation, and bounded value copy.

- [ ] **Step 2: Run RED**

Expected: missing split client functions.

- [ ] **Step 3: Implement one-operation state**

Store one connection handle, operation enum, copied UUID/range/value, one `gatt_client_notification_t`, and terminal guard. Initialize upstream GATT Client once. Reject a second operation with `BUSY` until `GATT_EVENT_QUERY_COMPLETE` or disconnect clears state.

- [ ] **Step 4: Map upstream events**

Map service, characteristic, descriptor, value, notification, indication, MTU, and query-complete events into compact internal events. Clamp copied value data to `BTSTACK_GATT_VALUE_MAX` and set `truncated` when native data is larger.

- [ ] **Step 5: Run client tests and disconnect/reentry mutations**

Expected: exactly one terminal event, state becomes idle before final callback delivery, and a callback-triggered next operation is not overwritten by the old stack frame.

---

### Task 7: Add LE lifecycle and HOGP to Profile Manager

**Files:**
- Modify: `btstack/include/core/btstack_profile_manager.h`
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/profiles/btstack_hogp_device.c`
- Modify: `btstack/include/profiles/btstack_hogp_device.h`
- Test: `btstack/core/tests/test_btstack_profile_manager_le.c`
- Test: `btstack/profiles/tests/test_btstack_hogp_device.c`

- [ ] **Step 1: Add RED mask and dependency cases**

Require:

```c
#define BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT (1u << 0)
#define BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER (1u << 1)
#define BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE (1u << 2)
#define BTSTACK_PROFILE_MANAGER_LE_ALL         0x07u
```

Test unsupported bits, disabled compile features, HOGP without Server, idempotent identical init, conflicting init, and deterministic init order Client → Server → HOGP.

- [ ] **Step 2: Observe RED**

Expected: current manager rejects every nonzero LE mask.

- [ ] **Step 3: Add one LE manager state**

Store role, peer/address type, connection handle, MTU, connected/encrypted/bonded flags, and current GATT Client state in one bounded structure protected by platform critical sections.

- [ ] **Step 4: Bind Profile events and cross-module cleanup**

On connect, pass the handle to Client/Server/HOGP. On encryption, update HOGP readiness. On disconnect, cancel Client, clear Server CCCD state, clear HOGP readiness, then emit the public disconnect event.

- [ ] **Step 5: Finish HOGP over GATT Server**

Register HID, Battery, and Device Information services through the new Server API. Preserve copied report map/PNP/report metadata. Mark ready only after connection, required encryption, and input-report subscription; send reports through the characteristic handle.

- [ ] **Step 6: Run manager and HOGP tests**

Expected: all lifecycle and compile-feature combinations pass with no Classic regression.

---

### Task 8: Add compact BLE requests/events and thin Core dispatch

**Files:**
- Modify: `btstack/core/btstack_core_internal.h`
- Modify: `btstack/core/btstack_core_dispatcher.c`
- Modify: `btstack/core/btstack_event_mapper.c`
- Create: `btstack/include/core/bt_xstack_le.h`
- Create: `btstack/include/core/bt_xstack_gatt.h`
- Test: `btstack/core/tests/test_btstack_ble_dispatch.c`
- Test: `btstack/core/tests/test_btstack_ble_event_size.ps1`

- [ ] **Step 1: Define RED request/event layout tests**

Add request IDs for LE adv/scan/connect/security and GATT Server/Client operations. Change request payload storage to a tagged union so the 244-byte GATT value does not add to every other payload field:

```c
typedef union {
    btstack_core_classic_request_t classic_request;
    bt_xstack_le_adv_config_t le_adv;
    bt_xstack_le_scan_config_t le_scan;
    btstack_core_le_connect_request_t le_connect;
    btstack_core_gatt_registration_request_t gatt_registration;
    btstack_core_gatt_value_request_t gatt_value;
} btstack_core_request_payload_t;
```

Extend the event envelope with `LE` and `GATT` union members and retain compile-time size guards against the fixed ring item size.

- [ ] **Step 2: Observe RED**

Run dispatcher compile/runtime tests. Expected: missing domains, request IDs, and payload union.

- [ ] **Step 3: Migrate existing request producers/consumers to the union**

Mechanically preserve all Classic/PBAP semantics first. Run existing API and AT tests before adding BLE dispatch cases.

- [ ] **Step 4: Add thin BLE switch cases**

Each case validates only request shape and calls exactly one Adapter/Profile Manager operation. No native `gap_*`, `sm_*`, `gatt_client_*`, or `att_server_*` call is allowed in Dispatcher.

- [ ] **Step 5: Add Event Mapper conversions**

Map internal GAP/GATT events into `type + union`, deep-copy bounded data, and post through existing critical/best-effort event rings. LE scan results use best-effort; connection, security, GATT operation completion, and Server writes use critical delivery.

- [ ] **Step 6: Run old and new dispatcher/event tests**

Expected: Classic/API/AT tests remain green and BLE size/admission tests pass.

---

### Task 9: Expose the public BLE API

**Files:**
- Create: `btstack/include/core/bt_xstack_le.h`
- Create: `btstack/include/core/bt_xstack_gatt.h`
- Modify: `btstack/include/core/bt_xstack.h`
- Modify: `btstack/api/bt_xstack_api_core.c`
- Test: `btstack/api/tests/test_bt_xstack_ble_api.c`
- Test: `btstack/api/tests/test_bt_xstack_ble_api.ps1`
- Test: `btstack/api/tests/test_bt_xstack_api_headers.c`
- Test: `btstack/api/tests/test_bt_xstack_api_headers.cpp`

- [ ] **Step 1: Add failing C and C++ header fixtures**

Require the exact public operation groups from the design: `bt_xstack_le_adv_*`, `bt_xstack_le_scan_*`, `bt_xstack_le_connect/disconnect/pair`, `bt_xstack_gatt_server_*`, and `bt_xstack_gatt_client_*`. Instantiate every public config/event union member in both C99 and C++11.

- [ ] **Step 2: Add a runtime admission/deep-copy RED test**

Stub the request queue boundary. Submit 31-byte advertising data and a 244-byte GATT write, mutate the caller buffers immediately, dequeue the request, and assert the queued bytes retain their original values. Verify 32-byte advertising and 245-byte GATT values return `INVALID_ARGUMENT` without queue submission.

- [ ] **Step 3: Implement public wrappers**

Each wrapper validates pointer/enum/length combinations, zeros one local request, copies bounded payload, sets the request ID, and submits non-blocking. No wrapper waits for protocol completion.

- [ ] **Step 4: Add callbacks to `bt_xstack_config_t`**

Add:

```c
bt_xstack_le_callback_t le_callback;
bt_xstack_gatt_callback_t gatt_callback;
```

Deliver them only from the existing Event Worker. Keep callback context shared with existing callbacks.

- [ ] **Step 5: Run C/C++/runtime API tests**

Use host GCC and target ARM GCC with `-Wall -Wextra -Werror -fsyntax-only`. Expected: all pass.

---

### Task 10: Add the BLE AT command and event surface

**Files:**
- Modify: `btstack/at/btstack_at_command_table.c`
- Modify: `btstack/at/btstack_at_dispatch_table.c`
- Modify: `btstack/at/btstack_at_parser.c`
- Modify: `btstack/at/btstack_at_protocol.c`
- Modify: `btstack/at/btstack_at_core_adapter.c`
- Modify: `btstack/at/btstack_at.c`
- Test: `btstack/at/tests/test_btstack_at_ble.c`
- Test: `btstack/at/tests/test_btstack_at_ble.ps1`

- [ ] **Step 1: Add RED table/parser cases**

Add concrete commands:

```text
AT+LEADV=<enable>,<interval_min>,<interval_max>,<type>,<hex_adv>,<hex_scan_rsp>
AT+LESCAN=<enable>,<active>,<interval>,<window>,<duplicate_filter>
AT+LECONN=<address_type>,<address>
AT+LEDISC
AT+LEPAIR
AT+LECFM=<accept>
AT+LEPASS=<passkey>
AT+GATTSADD=<service_uuid>
AT+GATTSC=<service_handle>,<characteristic_uuid>,<properties>,<max_len>,<security>
AT+GATTSBUILD
AT+GATTSVAL=<value_handle>,<hex_value>
AT+GATTSNTF=<value_handle>,<hex_value>
AT+GATTSIND=<value_handle>,<hex_value>
AT+GATTCDISC=<uuid_or_all>
AT+GATTCC=<service_start>,<service_end>,<uuid_or_all>
AT+GATTCR=<value_handle>
AT+GATTCW=<value_handle>,<with_response>,<hex_value>
AT+GATTCSUB=<value_handle>,<cccd_handle>,<mode>
```

- [ ] **Step 2: Observe RED**

Run parser/table/adapter tests. Expected: descriptors and mapping are absent.

- [ ] **Step 3: Implement strict text conversion**

Parse addresses, UUID16/UUID128, decimal fields, and even-length hex values into the same core request union used by API. Reject overflow before queue submission. Do not retain pointers into the input line.

- [ ] **Step 4: Format async events**

Format `+LEADV`, `+LESCAN`, `+LECONN`, `+LEPAIR`, `+GATTS`, and `+GATTC` from the shared event envelope. Hex output must be bounded by the existing AT output contract; set a `truncated` field rather than overrunning output storage.

- [ ] **Step 5: Run AT worker/ring/fairness tests**

Verify no callback occurs in producer/Run Loop context, event-ring overflow remains counted, BLE scan storms do not starve commands, and no 20 ms polling returns.

---

### Task 11: Integrate source sets, projects, packaged headers, and README

**Files:**
- Modify: `btstack/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Regenerate: the three `*_Debug.mk` files
- Modify: `btstack/README.md`
- Modify: `btstack/ports/README.md`
- Test: existing layout/build/package contracts plus `test_btstack_ble_build_contract.ps1`

- [ ] **Step 1: Strengthen project-set contracts and observe RED**

Derive expected common/front-end/port units from `Makefile.inc`. Require each project to contain every selected BLE source exactly once, deleted GAP/GATT units zero times, and exactly one selected frontend.

- [ ] **Step 2: Update the three projects and regenerate makefiles**

Use the official G+ generator already used by the project. Do not hand-edit generated compilation rules after generation.

- [ ] **Step 3: Run official API, AT, and Legacy forced builds**

Expected: exit 0, no new BLE/platform warning, archives newer than every selected BLE source/object.

- [ ] **Step 4: Regenerate packaged headers**

Require source/staged SHA equality for the full public header set, including `bt_xstack_le.h` and `bt_xstack_gatt.h`. Verify the API archive exports each new public function once and AT/Legacy archives contain the common BLE implementation once.

- [ ] **Step 5: Document usage**

Add one compiling API example and one AT sequence covering: register/build Server, enable, advertise, connect, read/write, notification, disconnect. Document single-link and Legacy BLE limits.

---

### Task 12: Full verification and target acceptance

**Files:**
- Test-only changes if a genuine regression exposes an obsolete contract
- No production edits unless a failing test identifies a root cause covered by the approved specification

- [ ] **Step 1: Run every BTstack PowerShell test independently**

Enumerate scripts under:

```text
btstack/api/tests
btstack/profiles/tests
btstack/core/tests
btstack/transport/tests
btstack/at/tests
btstack/gap/tests
```

Run each in a fresh PowerShell process and report pass/fail by directory plus the first failure. The known missing-patch contract must be reported separately rather than hidden.

- [ ] **Step 2: Force-build all three libraries**

Run official API and AT build scripts and the official Legacy generator + `make -B`. Capture warning categories and fail on any new BLE warning or any error.

- [ ] **Step 3: Force-link the main firmware**

Regenerate and run the main project `make -B -j4`. Verify the final ELF is newer than the API archive and all modified BLE sources.

- [ ] **Step 4: Audit artifacts**

Use ARM `ar`, `nm`, `size`, and the map file to verify:

```text
one btstack_platform_default per archive/ELF
one GAP LE implementation
one GATT Client implementation when enabled
one ATT/GATT Server implementation when enabled
no deleted GAP/GATT object
no duplicate frontend
no BLE task or dynamic queue
API ELF contains API resources and no AT resources
```

Record archive/ELF sizes and RAM/ROM deltas against the latest pre-BLE artifact evidence.

- [ ] **Step 5: Prepare board test checklist**

Run when hardware is available:

```text
Android and iPhone scan the configured legacy advertisement
Peripheral connection and Just Works Bond succeed
disconnect/reconnect in the same boot resolves the private address using RAM Bond
reset clears RAM Bond and requires a new pairing
GATT Server discovery/read/write/notification/indication succeed
Central scan/connect/discover/read/write/subscribe succeed
disconnect cancels one active GATT Client operation exactly once
HOGP input report arrives after encryption and CCCD subscription
```

Host and ARM build completion must explicitly retain a caveat until these board tests pass.
