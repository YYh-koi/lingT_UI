# Application-Owned Phonebook Synchronization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let applications start selected phone-book categories through intuitive public APIs while the stack retains internal PBAP terminology, one PBAP connection per mask, and no automatic sync on HFP `DEVICE`.

**Architecture:** Carry a validated public category mask through the existing xStack request `values[0]` slot into the generic PBAP wrapper. The wrapper skips unselected categories, performs one connection lifecycle, and emits its final completion only after native disconnect has reset the public and internal state to `IDLE`. The demo owns the policy of requesting `BT_XSTACK_PHONEBOOK_ALL` when it receives HFP `DEVICE`; HFP `LOST` cleanup remains internal.

**Tech Stack:** C99/C++11 public headers, FreeRTOS xStack queues/tasks, BTstack 1.6.2 PBAP/GOEP/OBEX, PowerShell contract tests, MinGW host tests, G+ ARM GCC build.

---

The workspace has no Git metadata. The commit steps normally required by this workflow are replaced by focused verification checkpoints; no synthetic repository will be created.

### Task 1: Define and test generic PBAP category selection

**Files:**
- Modify: `btstack_api/include/profiles/btstack_pbap_client.h`
- Modify: `btstack_api/profiles/btstack_pbap_client.c`
- Modify: `btstack_api/profiles/tests/test_btstack_pbap_state_machine.c`
- Modify: `btstack_api/profiles/tests/test_btstack_pbap_profile_contract.ps1`

- [ ] **Step 1: Write failing profile tests for masks and selected ordering**

Extend the host state-machine test so its sync helper accepts a mask and add cases equivalent to:

```c
#define CONTACTS_MASK (1u << BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS)
#define INCOMING_MASK (1u << BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING)
#define OUTGOING_MASK (1u << BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING)
#define MISSED_MASK (1u << BTSTACK_PBAP_CLIENT_CATEGORY_MISSED)

CHECK(btstack_pbap_client_sync(peer, 0u) ==
      BTSTACK_STATUS_INVALID_ARGUMENT);
CHECK(btstack_pbap_client_sync(peer, 0x80u) ==
      BTSTACK_STATUS_INVALID_ARGUMENT);

begin_sync_with_mask(peer, CONTACTS_MASK | MISSED_MASK);
CHECK(strcmp(last_pull_path(), "telecom/pb.vcf") == 0);
complete_current_pull(ERROR_CODE_SUCCESS);
CHECK(strcmp(last_pull_path(), "telecom/mch.vcf") == 0);
complete_current_pull(ERROR_CODE_SUCCESS);
CHECK(pull_calls == 2);
CHECK(disconnect_calls == 1);
```

Add an `ALL` case that proves `pb.vcf`, `ich.vcf`, `och.vcf`, and `mch.vcf` are requested once and in that order through one native connection. Update existing calls to pass `BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL`.

- [ ] **Step 2: Run the profile tests and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_pbap_state_machine.ps1
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_pbap_profile_contract.ps1
```

Expected: compile/contract failure because `btstack_pbap_client_sync` has no mask parameter or category-mask constants and the implementation still pulls all four categories.

- [ ] **Step 3: Add the internal professional mask API**

In the generic profile header, keep singular event categories and add internal mask definitions:

```c
typedef uint8_t btstack_pbap_client_category_mask_t;

#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS \
    ((btstack_pbap_client_category_mask_t)(1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_INCOMING \
    ((btstack_pbap_client_category_mask_t)(1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_OUTGOING \
    ((btstack_pbap_client_category_mask_t)(1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_MISSED \
    ((btstack_pbap_client_category_mask_t)(1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_MISSED))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL \
    ((btstack_pbap_client_category_mask_t)0x0fu)

int btstack_pbap_client_sync(
    const btstack_bd_addr_t address,
    btstack_pbap_client_category_mask_t category_mask);
```

Store `category_mask` in the wrapper state. Validate nonzero/no-unknown-bit input before the busy check, select the first set category before `pbap_connect`, and replace unconditional increment with a bounded next-selected-category search:

```c
static uint8_t category_is_selected(uint8_t category)
{
    return (pbap.category_mask & (uint8_t)(1u << category)) != 0u;
}

static uint8_t select_next_category(void)
{
    do {
        ++pbap.category_index;
    } while ((pbap.category_index < PBAP_CATEGORY_COUNT) &&
             !category_is_selected(pbap.category_index));
    return pbap.category_index < PBAP_CATEGORY_COUNT;
}
```

Do not reset `category_index` to contacts after the native connection opens. Preserve the existing category event, parser, flow-control, failure continuation, and backpressure behavior.

- [ ] **Step 4: Run the profile tests and verify GREEN**

Run the two commands from Step 2.

Expected: state-machine groups and profile contract pass, including subset order, all-category order, invalid masks, unsupported-category continuation, and existing re-entry/backpressure cases.

- [ ] **Step 5: Verification checkpoint**

Run:

```powershell
rg -n "category_mask|CATEGORY_MASK_ALL|select_next_category" btstack_api\include\profiles\btstack_pbap_client.h btstack_api\profiles\btstack_pbap_client.c
```

Expected: one mask field, one public generic profile signature, and no change to native `pbap_*` naming.

### Task 2: Delay final completion until PBAP is idle

**Files:**
- Modify: `btstack_api/profiles/btstack_pbap_client.c`
- Modify: `btstack_api/profiles/tests/test_btstack_pbap_state_machine.c`

- [ ] **Step 1: Write a failing completion-order and chained-sync test**

Add a callback assertion that records the wrapper state observed at final completion and starts a second synchronization inline:

```c
if (event->type == BTSTACK_PBAP_CLIENT_EVENT_COMPLETED) {
    state_seen_at_completed = btstack_pbap_client_get_state();
    chained_sync_result = btstack_pbap_client_sync(
        chained_peer, BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
}
```

Drive the first selected category to native operation completion but do not send the native connection-closed event. Assert no final event yet. Then send connection closed and assert:

```c
CHECK(state_seen_at_completed == BTSTACK_PBAP_CLIENT_STATE_IDLE);
CHECK(chained_sync_result == BTSTACK_STATUS_OK);
CHECK(connect_calls == 2);
```

- [ ] **Step 2: Run the state-machine test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_pbap_state_machine.ps1
```

Expected: failure because current `finish_all_categories()` emits `COMPLETED` while state is `DISCONNECTING`, before native close.

- [ ] **Step 3: Route normal success through terminal cleanup**

Replace the current early final emission in `finish_all_categories()` with the existing cleanup state machine:

```c
static void finish_all_categories(void)
{
    btstack_pbap_client_status_t status = pbap.any_category_failed ?
        BTSTACK_PBAP_CLIENT_STATUS_PARTIAL :
        BTSTACK_PBAP_CLIENT_STATUS_SUCCESS;

    start_terminal_cleanup(status,
        BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
}
```

The connection-closed handler must call `emit_final(..., 1u)`, which resets internal state, invokes the state callback so the xStack cache becomes `IDLE`, and only then delivers final completion. Preserve the single-final-event guard and late-event/CID filtering.

- [ ] **Step 4: Run the state-machine test and verify GREEN**

Run the Step 2 command.

Expected: all state-machine groups pass, no final completion exists before native close, callback observes `IDLE`, and inline chained sync connects successfully.

- [ ] **Step 5: Re-run parser and profile contracts**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_vcard_parser.ps1
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_pbap_profile_contract.ps1
```

Expected: parser cases and profile contract pass unchanged.

### Task 3: Expose intuitive public phone-book names and carry the mask

**Files:**
- Modify: `btstack_api/include/api/bt_xstack_phonebook.h`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_event.c`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_headers.c`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_headers.cpp`
- Modify: `btstack_api/api/tests/test_bt_xstack_pbap_contract.ps1`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_contract.ps1`

- [ ] **Step 1: Write failing public API and transport contracts**

Require these public declarations and forbid old public PBAP identifiers:

```c
typedef uint8_t bt_xstack_phonebook_category_mask_t;

enum {
    BT_XSTACK_PHONEBOOK_CONTACTS = 1u << 0,
    BT_XSTACK_PHONEBOOK_INCOMING = 1u << 1,
    BT_XSTACK_PHONEBOOK_OUTGOING = 1u << 2,
    BT_XSTACK_PHONEBOOK_MISSED   = 1u << 3,
    BT_XSTACK_PHONEBOOK_ALL =
        BT_XSTACK_PHONEBOOK_CONTACTS |
        BT_XSTACK_PHONEBOOK_INCOMING |
        BT_XSTACK_PHONEBOOK_OUTGOING |
        BT_XSTACK_PHONEBOOK_MISSED
};

bt_xstack_result_t bt_xstack_phonebook_sync(
    const bt_xstack_addr_t *address,
    bt_xstack_phonebook_category_mask_t categories);
bt_xstack_result_t bt_xstack_phonebook_cancel(void);
bt_xstack_result_t bt_xstack_phonebook_authenticate(const char *password);
bt_xstack_result_t bt_xstack_phonebook_get_state(
    bt_xstack_phonebook_state_t *state);
```

Compile C and C++ header fixtures that use each bit, a combined mask, and `ALL`. Contract assertions must require `request.values[0] = categories`, backend forwarding of `request->values[0]`, zero/unknown-bit validation, and the absence of public `bt_xstack_pbap_*`, `bt_xstack_get_pbap_state`, `bt_xstack_pbap_state_t`, `BT_XSTACK_PBAP_STATE_*`, and `BT_XSTACK_PBAP_PASSWORD_MAX`.

- [ ] **Step 2: Run API contracts and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_pbap_contract.ps1
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_api_contract.ps1
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_api_headers.ps1
```

Expected: failures on missing phone-book command/state names, missing mask forwarding, and remaining public PBAP names.

- [ ] **Step 3: Rename the public domain and validate masks**

Rename only application-facing types/constants/functions to `PHONEBOOK`; keep internal request IDs, cached-state helper names, backend calls, generic wrapper names, and native calls as PBAP. Implement public sync admission as:

```c
bt_xstack_result_t bt_xstack_phonebook_sync(
    const bt_xstack_addr_t *address,
    bt_xstack_phonebook_category_mask_t categories)
{
    bt_xstack_api_request_t request;

    if ((address == NULL) || (categories == 0u) ||
        ((categories & (bt_xstack_phonebook_category_mask_t)
            ~BT_XSTACK_PHONEBOOK_ALL) != 0u)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_SYNC;
    memcpy(&request.address, address, sizeof(*address));
    request.has_address = 1u;
    request.values[0] = categories;
    request.value_count = 1u;
    return bt_xstack_api_submit(&request);
}
```

Forward the value in the backend:

```c
status = btstack_pbap_client_sync(address,
    (btstack_pbap_client_category_mask_t)request->values[0]);
```

Rename public state casts and cached state storage types consistently without renaming `bt_xstack_api_state_read_pbap()` or `bt_xstack_api_state_write_pbap()`.

- [ ] **Step 4: Run API contracts and verify GREEN**

Run the three commands from Step 2.

Expected: all public/API/header tests pass under C99 and C++11 with `-Wall -Wextra -Werror`.

- [ ] **Step 5: Verify public/internal naming boundary**

Run:

```powershell
rg -n "bt_xstack_pbap_|bt_xstack_get_pbap_state|BT_XSTACK_PBAP_STATE|BT_XSTACK_PBAP_PASSWORD" btstack_api\include\api btstack_api\api\tests\test_bt_xstack_api_headers.*
rg -n "BT_XSTACK_REQUEST_PBAP|btstack_pbap_client_|\bpbap_" btstack_api\api btstack_api\profiles
```

Expected: first command finds nothing; second confirms professional names remain inside stack layers.

### Task 4: Move the HFP `DEVICE` policy into the demo

**Files:**
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/tests/test_bt_xstack_pbap_queue_contract.ps1`
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Write failing ownership and demo contracts**

The queue contract must isolate the phone event delivery branch and forbid `BT_XSTACK_REQUEST_PBAP_SYNC` in the `DEVICE` path while still requiring `BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED` plus reliable `portMAX_DELAY` submission in the `LOST` path.

The demo contract must require this call inside the phone callback's `DEVICE` branch:

```c
status = bt_xstack_phonebook_sync(
    &event->address, BT_XSTACK_PHONEBOOK_ALL);
DBG_PRINT("xstack demo phonebook sync status=%d\r\n", status);
```

- [ ] **Step 2: Run both contracts and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_pbap_queue_contract.ps1
powershell.exe -NoProfile -File Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
```

Expected: xStack still auto-submits sync and the demo does not call the new API.

- [ ] **Step 3: Remove automatic sync and add demo policy**

Delete only the HFP `DEVICE` internal request branch from `bt_xstack_api_deliver_event()`. Keep application phone callback delivery first and keep the existing reliable HFP `LOST` cleanup request unchanged.

In `bt_xstack_demo_phone_event()`, after the event is logged and before entering its state-update critical section, add:

```c
if (event->id == BT_XSTACK_PHONE_EVENT_DEVICE) {
    bt_xstack_result_t status = bt_xstack_phonebook_sync(
        &event->address, BT_XSTACK_PHONEBOOK_ALL);
    DBG_PRINT("xstack demo phonebook sync status=%d\r\n", status);
}
```

Do not hold a critical section while submitting the application request. Do not add retries, a queue, or a task; a queue-admission failure is visible in the log and remains application policy.

- [ ] **Step 4: Run both contracts and verify GREEN**

Run the Step 2 commands.

Expected: both pass; DEVICE is delivery-only in core, demo owns `ALL`, and LOST cleanup remains reliable/FIFO.

- [ ] **Step 5: Re-run the focused cross-layer contracts**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_pbap_contract.ps1
powershell.exe -NoProfile -File btstack_api\profiles\tests\test_btstack_pbap_state_machine.ps1
```

Expected: API bridge and generic profile state-machine tests pass.

### Task 5: Update documentation, package contracts, and staged API

**Files:**
- Modify: `btstack_api/README_API.md`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_package_contract.ps1`
- Modify: `btstack_api/api/tests/test_bt_xstack_pbap_build_package_contract.ps1`
- Generated/staged: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/bt_xstack_phonebook.h`
- Generated: `Project/WirelessLibraries/BTstack_API_Library/GPCV2178_BTstack_API_Library_Debug.mk`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`

- [ ] **Step 1: Write failing documentation/package assertions**

Require the README and packaged header/archive to contain:

```text
bt_xstack_phonebook_sync
BT_XSTACK_PHONEBOOK_CONTACTS
BT_XSTACK_PHONEBOOK_INCOMING
BT_XSTACK_PHONEBOOK_OUTGOING
BT_XSTACK_PHONEBOOK_MISSED
BT_XSTACK_PHONEBOOK_ALL
```

Forbid old public PBAP command/state symbols in the README, staged public header, and archive. Require documentation to state that applications trigger sync, masks share one PBAP connection, category order is contacts/incoming/outgoing/missed, `COMPLETED` means `IDLE`, and HFP `LOST` cleanup remains internal.

- [ ] **Step 2: Run package contracts and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_api_package_contract.ps1
powershell.exe -NoProfile -File btstack_api\api\tests\test_bt_xstack_pbap_build_package_contract.ps1
```

Expected: stale README, staged header, and archive still expose old names.

- [ ] **Step 3: Update README with the application flow**

Document this example and its asynchronous semantics:

```c
if (event->id == BT_XSTACK_PHONE_EVENT_DEVICE) {
    (void)bt_xstack_phonebook_sync(
        &event->address,
        BT_XSTACK_PHONEBOOK_CONTACTS |
        BT_XSTACK_PHONEBOOK_MISSED);
}
```

State explicitly that `ALL` uses one PBAP connection, records must be copied during the callback if persisted, and no PBAP-specific event queue/task exists.

- [ ] **Step 4: Rebuild and stage the ARM library/package**

Run:

```powershell
powershell.exe -NoProfile -File Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1 -Jobs 4
```

Expected: generator and ARM build succeed, `libxstack_api.a` is refreshed, and six public headers including `bt_xstack_phonebook.h` are staged.

- [ ] **Step 5: Run package contracts and verify GREEN**

Run the Step 2 commands.

Expected: archive exports the four new phone-book APIs, does not export old public PBAP APIs, internal `btstack_pbap_client_sync`, native PBAP, GOEP/OBEX, MD5, and yxml symbols remain present, and the staged header matches the source header.

### Task 6: Full verification and firmware build

**Files:**
- Verify all modified and generated files above
- Build: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release_Debug.mk`

- [ ] **Step 1: Run every API and profile PowerShell test**

Run:

```powershell
$scripts = @(
    Get-ChildItem btstack_api\api\tests -Filter *.ps1 -File
    Get-ChildItem btstack_api\profiles\tests -Filter *.ps1 -File
)
foreach ($script in $scripts) {
    & powershell.exe -NoProfile -File $script.FullName
    if ($LASTEXITCODE -ne 0) { throw "FAILED: $($script.FullName)" }
}
"PASS: $($scripts.Count)/$($scripts.Count) API/profile scripts"
```

Expected: every script passes, including parser, upstream hash, queue, public header, profile state machine, and build/package contracts.

- [ ] **Step 2: Compile the modified ARM units with project flags**

Use the generated BTstack API makefile through the full build command from Task 5. Confirm that `bt_xstack_api_core.c`, `bt_xstack_api_backend.c`, `bt_xstack_api_event.c`, and `btstack_pbap_client.c` compile without new warnings or errors.

Expected: exit code 0; the known unrelated `avdtp.c:339` maybe-uninitialized warning may remain.

- [ ] **Step 3: Build the main firmware including the demo**

Run from `Project/GPCV2178_Turnkey_SDK`:

```powershell
$env:Path = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;D:\mingw32\bin;' + $env:Path
& 'D:\mingw32\bin\mingw32-make.exe' `
    -f GPCV2178_turnkey_sdk_release_Debug.mk -j4
if ($LASTEXITCODE -ne 0) { throw 'Main firmware build failed' }
```

Expected: exit code 0 and the demo compiles against the new public API. The existing `PRINT_BUF_SIZE` redefinition warning may remain if unchanged by this feature.

- [ ] **Step 4: Inspect final symbols and stale names**

Run:

```powershell
$nm = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe'
& $nm -g --defined-only gplib\liba_wifi_src\atbm\atbm6165\bt_api\libxstack_api.a |
    Select-String 'bt_xstack_(phonebook|pbap)|btstack_pbap_client_sync'
rg -n "bt_xstack_pbap_|bt_xstack_get_pbap_state|BT_XSTACK_PBAP_STATE|BT_XSTACK_PBAP_PASSWORD" btstack_api Project\wifi_demo gplib\liba_wifi_src\atbm\atbm6165\bt_api -g '!*.a'
```

Expected: archive contains `bt_xstack_phonebook_sync`, `cancel`, `authenticate`, and `get_state`, retains internal `btstack_pbap_client_sync`, and source/staged public surfaces contain no obsolete public PBAP names except negative test assertions or historical design documents.

- [ ] **Step 5: Report target-only validation boundary**

Record that desktop/ARM verification is complete. Do not claim Android/iPhone interoperability, live category-mask selection, permission-denial behavior, ROM/RAM delta, queue depth, or timing until a refreshed firmware is run on the board and a new serial log is reviewed.
