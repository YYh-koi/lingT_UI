# BTstack PBAP Client Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add BTstack 1.6.2 PBAP PCE support that downloads contacts and call history, parses complete vCards on the BTstack run loop, and delivers one structured record at a time through the existing xStack event queues.

**Architecture:** Exact upstream PBAP/GOEP/OBEX/MD5/yxml sources remain in `btstack_api/src` and `btstack_api/3rd-party`. A bounded parser and generic `profiles/btstack_pbap_client.c` wrapper own protocol state on the run loop. The typed xStack layer adds commands and a phone-book event domain; PBAP record events use the existing critical queue with four reserved control slots and edge-triggered resume.

**Tech Stack:** C11-compatible embedded C, BTstack 1.6.2, FreeRTOS static queues/tasks, PowerShell contract tests, G+ ARM toolchain.

---

## File structure

- `btstack_api/src/classic/*.c`: exact upstream native PBAP/GOEP/OBEX implementation.
- `btstack_api/3rd-party/md5/*`, `btstack_api/3rd-party/yxml/*`: exact upstream dependencies and licenses.
- `btstack_api/profiles/btstack_vcard_parser.{c,h}`: bounded streaming parser, independently testable.
- `btstack_api/include/profiles/btstack_pbap_client.h`: generic profile state, record and callback contract.
- `btstack_api/profiles/btstack_pbap_client.c`: PBAP category state machine, flow control, pending record and replay buffer.
- `btstack_api/include/api/bt_xstack_phonebook.h`: application-facing PBAP commands, states and events.
- `btstack_api/api/bt_xstack_api_{core,backend,event,internal}.c/.h`: command routing, event bridge and queue reserve.
- `btstack_api/**/tests`: source, parser, profile, queue and API contract tests.
- `Project/WirelessLibraries/BTstack_API_Library/*`, `btstack_api/adapter/Makefile.inc`, `tools/wireless_lib/wireless_sources.json`: build/package integration.

### Task 1: Import exact BTstack 1.6.2 protocol sources

**Files:**
- Create: `btstack_api/src/classic/pbap_client.c`
- Create: `btstack_api/src/classic/goep_client.c`
- Create: `btstack_api/src/classic/obex_message_builder.c`
- Create: `btstack_api/src/classic/obex_parser.c`
- Create: `btstack_api/src/classic/obex_iterator.c`
- Create: `btstack_api/src/classic/obex_srm_client.c`
- Create: `btstack_api/3rd-party/md5/md5.c`
- Create: `btstack_api/3rd-party/md5/md5.h`
- Create: `btstack_api/3rd-party/yxml/yxml.c`
- Create: `btstack_api/3rd-party/yxml/yxml.h`
- Preserve: the license notice embedded in upstream `md5.c`/`md5.h`
- Create: `btstack_api/3rd-party/yxml/COPYING`
- Create: `btstack_api/profiles/tests/test_btstack_pbap_upstream_contract.ps1`

- [ ] **Step 1: Write the failing source contract**

The test declares the exact tag and required paths, verifies every file exists, rejects duplicate source references, and checks the actual upstream dependency closure: `pbap_client.c` includes `md5.h` and `classic/pbap_client.h`, while `pbap_client.h` includes `yxml.h` and `classic/obex_srm_client.h`.

```powershell
$required = @(
  'src/classic/pbap_client.c', 'src/classic/goep_client.c',
  'src/classic/obex_message_builder.c', 'src/classic/obex_parser.c',
  'src/classic/obex_iterator.c', 'src/classic/obex_srm_client.c',
  '3rd-party/md5/md5.c', '3rd-party/md5/md5.h',
  '3rd-party/yxml/yxml.c', '3rd-party/yxml/yxml.h')
foreach ($path in $required) {
  if (-not (Test-Path (Join-Path $root $path))) { throw "missing $path" }
}
```

- [ ] **Step 2: Run the test and verify RED**

Run: `pwsh -File btstack_api/profiles/tests/test_btstack_pbap_upstream_contract.ps1`

Expected: FAIL with `missing src/classic/pbap_client.c`.

- [ ] **Step 3: Fetch exact upstream files**

Download from `https://raw.githubusercontent.com/bluekitchen/btstack/v1.6.2/` without modifying copyright blocks. Compare existing headers against the same tag and replace only when their hashes differ. Preserve the MD5 embedded license notice and the yxml `COPYING` file exactly; do not invent or rename license files absent from the upstream tag.

- [ ] **Step 4: Record and verify SHA-256 hashes**

Add the resulting hashes to the contract test and verify all imported files match the immutable `v1.6.2` raw content.

- [ ] **Step 5: Run the source contract and verify GREEN**

Run: `pwsh -File btstack_api/profiles/tests/test_btstack_pbap_upstream_contract.ps1`

Expected: PASS with one source copy per required implementation.

### Task 2: Implement the bounded streaming vCard parser

**Files:**
- Create: `btstack_api/profiles/btstack_vcard_parser.h`
- Create: `btstack_api/profiles/btstack_vcard_parser.c`
- Create: `btstack_api/profiles/tests/test_btstack_vcard_parser.c`
- Create: `btstack_api/profiles/tests/test_btstack_vcard_parser.ps1`

- [ ] **Step 1: Write failing parser tests**

Use the wished-for API below and test vCard 2.1/3.0, split input at every byte, folded lines, escaped values, UTF-8 Chinese names, quoted-printable text, four numbers, excess-number truncation, call direction/timestamp, ignored PHOTO data, and malformed-card resynchronization.

```c
btstack_vcard_parser_t parser;
btstack_vcard_record_t record;
btstack_vcard_parser_init(&parser, record_callback, &record);
btstack_vcard_parser_process(&parser, chunk, chunk_len);
btstack_vcard_parser_finish(&parser);
```

The public parser record has these fixed capacities:

```c
#define BTSTACK_VCARD_NAME_MAX 48u
#define BTSTACK_VCARD_NUMBER_MAX 32u
#define BTSTACK_VCARD_NUMBER_COUNT 4u
#define BTSTACK_VCARD_TIMESTAMP_MAX 24u
```

- [ ] **Step 2: Run tests and verify RED**

Run: `pwsh -File btstack_api/profiles/tests/test_btstack_vcard_parser.ps1`

Expected: compilation fails because `btstack_vcard_parser.h` is absent.

- [ ] **Step 3: Implement line streaming and unfolding**

Keep one bounded logical-line buffer, handle CRLF split across chunks and continuation lines beginning with space/tab, and stop at record boundaries. No heap allocation is allowed.

- [ ] **Step 4: Implement property decoding**

Parse property name/parameters before `:`, decode `\\n`, `\\,`, `\\;`, `\\\\`, quoted-printable hex and soft line breaks, and copy valid UTF-8 bytes without transcoding. Extract `FN`, fallback `N`, `TEL`, and `X-IRMC-CALL-DATETIME` qualifiers only.

- [ ] **Step 5: Implement bounded record emission and recovery**

Emit only after `END:VCARD`; mark truncation for clipped fields/excess numbers, skip malformed records, increment parser errors, and resynchronize on the next `BEGIN:VCARD`.

- [ ] **Step 6: Run parser tests and verify GREEN**

Run: `pwsh -File btstack_api/profiles/tests/test_btstack_vcard_parser.ps1`

Expected: all parser cases pass under the ARM compiler with warnings treated as errors.

### Task 3: Add the generic PBAP profile wrapper

**Files:**
- Create: `btstack_api/include/profiles/btstack_pbap_client.h`
- Create: `btstack_api/profiles/btstack_pbap_client.c`
- Create: `btstack_api/profiles/tests/test_btstack_pbap_profile_contract.ps1`
- Create: `btstack_api/profiles/tests/test_btstack_pbap_state_machine.c`
- Create: `btstack_api/profiles/tests/test_btstack_pbap_state_machine.ps1`
- Modify: `btstack_api/include/profiles/btstack_classic_profiles.h`
- Modify: `btstack_api/profiles/btstack_classic_profiles.c`
- Modify: `btstack_api/ports/gpcv2178/btstack_config.h`

- [ ] **Step 1: Write failing profile/state-machine tests**

Define a fake native PBAP seam and assert connect, authentication, category order, unsupported-category continuation, cancellation, HFP loss, record backpressure and single edge-triggered resume.

```c
int btstack_pbap_client_sync(const btstack_bd_addr_t address);
int btstack_pbap_client_cancel(void);
int btstack_pbap_client_authenticate(const char *password);
btstack_pbap_client_state_t btstack_pbap_client_get_state(void);
void btstack_pbap_client_event_space_available(void);
```

Expected pull order:

```text
telecom/pb.vcf -> telecom/ich.vcf -> telecom/och.vcf -> telecom/mch.vcf
```

- [ ] **Step 2: Run tests and verify RED**

Run: `pwsh -File btstack_api/profiles/tests/test_btstack_pbap_state_machine.ps1`

Expected: FAIL because the wrapper API is missing.

- [ ] **Step 3: Implement setup and native event translation**

Initialize native `pbap_client_init()`, use one static context/PBAP CID, enable explicit flow control after connection, and translate native connection/authentication/operation/data events into generic profile events.

- [ ] **Step 4: Implement category sequencing and failures**

Pull the four paths in order. An individual non-success operation emits category completion and advances; fatal connection, authentication timeout, cancellation or disconnect emits one final completion.

- [ ] **Step 5: Implement parser backpressure**

Store one complete pending profile event and up to `HCI_ACL_PAYLOAD_SIZE` unconsumed bytes. When the callback rejects a record, stop at its boundary and do not call `pbap_next_packet()`. Resume only from one scheduled run-loop callback after queue space becomes available.

- [ ] **Step 6: Wire setup and configuration**

Add `BTSTACK_ENABLE_PBAP_CLIENT 1`, initialize the wrapper from classic setup, and keep all native calls on the BTstack run loop.

- [ ] **Step 7: Run profile tests and verify GREEN**

Run both profile scripts. Expected: state-machine and static-boundary contracts pass with no heap allocation and no PBAP-specific queue/task.

### Task 4: Integrate PBAP with the typed xStack API and general event queue

**Files:**
- Create: `btstack_api/include/api/bt_xstack_phonebook.h`
- Create: `btstack_api/api/tests/test_bt_xstack_pbap_contract.ps1`
- Create: `btstack_api/api/tests/test_bt_xstack_pbap_queue_contract.ps1`
- Modify: `btstack_api/include/api/bt_xstack.h`
- Modify: `btstack_api/include/api/bt_xstack_types.h`
- Modify: `btstack_api/api/bt_xstack_api_internal.h`
- Modify: `btstack_api/api/bt_xstack_api_core.c`
- Modify: `btstack_api/api/bt_xstack_api_backend.c`
- Modify: `btstack_api/api/bt_xstack_api_event.c`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_headers.c`
- Modify: `btstack_api/api/tests/test_bt_xstack_api_headers.cpp`

- [ ] **Step 1: Write failing public API and queue tests**

The public header declares:

```c
bt_xstack_result_t bt_xstack_pbap_sync(const bt_xstack_addr_t *address);
bt_xstack_result_t bt_xstack_pbap_cancel(void);
bt_xstack_result_t bt_xstack_pbap_authenticate(const char *password);
bt_xstack_result_t bt_xstack_get_pbap_state(bt_xstack_pbap_state_t *state);
```

The event callback receives STARTED, CONTACT, CALL, CATEGORY_COMPLETED, COMPLETED, AUTHENTICATION_REQUIRED and ERROR events. The test compiles C/C++ consumers and asserts:

```c
_Static_assert(sizeof(bt_xstack_phonebook_event_t) <=
               sizeof(bt_xstack_audio_event_t),
               "PBAP must not enlarge queue slots");
```

- [ ] **Step 2: Run tests and verify RED**

Run: `pwsh -File btstack_api/api/tests/test_bt_xstack_pbap_contract.ps1`

Expected: FAIL because `bt_xstack_phonebook.h` is missing.

- [ ] **Step 3: Add typed requests and state getter**

Add SYNC/CANCEL/AUTH request IDs, validate address/password, copy request data into existing request slots, dispatch through the existing prepared-command run-loop callback, and protect the cached PBAP state with the existing critical-section pattern.

- [ ] **Step 4: Add the phone-book event domain**

Add `phonebook_callback` to `bt_xstack_config_t`, one union member to `bt_xstack_api_event_envelope_t`, event translation in `bt_xstack_api_event.c`, and callback delivery on the existing event task.

- [ ] **Step 5: Add record reserve without another queue**

Define `BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE 4u`. Before posting CONTACT/CALL records, require `uxQueueSpacesAvailable(critical_queue) > reserve`; return a distinct backpressure result without incrementing drop counters. Lifecycle/control events continue using the normal critical path.

- [ ] **Step 6: Add edge-triggered resume and automatic sync**

After the event task frees critical queue space, call `btstack_pbap_client_event_space_available()`. Its wrapper guard schedules at most one run-loop resume. After delivering an HFP CONNECTED event, submit the same internal request used by manual sync; after HFP loss, submit cancellation.

- [ ] **Step 7: Run API and queue tests and verify GREEN**

Run all `btstack_api/api/tests/*.ps1`. Expected: public C/C++ contracts pass, queue lengths remain 16/16, no PBAP queue/task exists, reserve is four, and the envelope size is unchanged.

### Task 5: Build, package, and verify the complete integration

**Files:**
- Modify: `btstack_api/adapter/Makefile.inc`
- Modify: `btstack_api/src/classic/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Regenerate: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library_Debug.mk`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1`
- Modify: `tools/wireless_lib/wireless_sources.json`
- Create: `btstack_api/api/tests/test_bt_xstack_pbap_build_integration.ps1`
- Update: `btstack_api/README_API.md`

- [ ] **Step 1: Write the failing build/package contract**

Assert every PBAP/GOEP/OBEX/MD5/yxml/parser/wrapper source appears exactly once in the API library project and generated makefile, the include paths contain MD5/yxml, and `bt_xstack_phonebook.h` is staged with the other public headers.

- [ ] **Step 2: Run build contract and verify RED**

Run: `pwsh -File btstack_api/api/tests/test_bt_xstack_pbap_build_integration.ps1`

Expected: FAIL listing missing project units.

- [ ] **Step 3: Update source manifests and project**

Add the exact sources/include paths to `Makefile.inc`, the G+ project XML and wireless manifest once. Update expected source counts consistently. Regenerate the makefile using `gpcmprj2make.exe`; do not hand-edit generated dependency output.

- [ ] **Step 4: Update packaging and documentation**

Stage `bt_xstack_phonebook.h`, document callback ownership, automatic/manual synchronization, authentication, truncation and application-owned storage. Correct the pre-existing Profile contract scripts to address `btstack_api/...` in this checkout.

- [ ] **Step 5: Run all focused contracts**

Run parser, profile, API, project and packaging scripts. Expected: zero failures; any unrelated pre-existing SDK tests are reported separately.

- [ ] **Step 6: Perform a clean ARM library build**

Run:

```powershell
pwsh -File Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1
```

Expected: compiler/linker exit 0, `libxstack_api.a` rebuilt, and six public xStack headers staged.

- [ ] **Step 7: Verify final requirements**

Confirm no new FreeRTOS queue/task, event queue slot size unchanged, every source compiled once, no heap added by wrapper/parser, and tests cover byte-split vCards and record backpressure. Record that physical Android/iPhone interoperability, timing and ROM/RAM measurements remain target-board validation steps if hardware is unavailable.
