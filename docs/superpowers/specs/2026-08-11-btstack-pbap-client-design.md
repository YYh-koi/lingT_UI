# BTstack PBAP Client Integration Design

## Objective

Add a Phone Book Access Profile Client Equipment (PBAP PCE) to the
GPCV2178 BTstack integration. The device pulls contacts and call history from
the connected phone's Phone Book Server Equipment (PSE), parses vCard data on
the BTstack run-loop task, and reports one complete structured record at a time
to the application through the existing asynchronous xStack event path. The
application owns storage and persistence.

The first release supports:

- automatic synchronization after the HFP service-level connection becomes
  active;
- manual resynchronization and cancellation;
- contacts plus incoming, outgoing, and missed call history;
- structured contact and call-history events;
- PBAP flow control and lossless backpressure;
- OBEX authentication when requested by the phone.

## Current State

The repository uses BTstack 1.6.2. It contains PBAP, GOEP, and OBEX public
headers, but not the corresponding PBAP implementation sources or the MD5 and
yxml dependencies. The generated G+ project currently compiles HFP HF, A2DP
Sink, and AVRCP but has no PBAP compilation units.

The public `bt_xstack` API currently exposes service, connection, phone, and
audio domains. Its asynchronous application delivery uses existing static
critical and best-effort event queues. PBAP must reuse this event pipeline; it
must not add a PBAP-specific queue or task.

## Considered Approaches

### Directly expose native PBAP APIs

This is the smallest wrapper, but it leaks BTstack connection identifiers,
native packets, and run-loop ownership into the application. The application
would also need to parse vCard streams and handle PBAP flow control. This does
not match the existing xStack abstraction.

### Copy the upstream PBAP demo into the application

This proves protocol operation quickly, but mixes protocol state, vCard
parsing, application storage, and board behavior. It is unsuitable as a
reusable SDK interface.

### Layered PBAP profile and xStack integration (selected)

Import the exact BTstack 1.6.2 PBAP dependencies, add a generic PBAP profile
wrapper and bounded vCard parser, and expose asynchronous xStack commands and
events. This preserves the existing task and API ownership model and keeps
application persistence out of the Bluetooth stack.

## Architecture

```text
HFP connected event / manual sync request
                  |
                  v
        xStack request queue
                  |
                  v
        xStack command task
                  |
                  v
        BTstack run-loop task
                  |
                  v
 PBAP profile -> PBAP/GOEP/OBEX -> phone PSE
                  |
          PBAP_DATA_PACKET
                  |
                  v
 streaming vCard parser on the run-loop task
                  |
       complete structured record
                  |
                  v
 existing xStack asynchronous event queue
                  |
                  v
         xStack event task
                  |
                  v
 application callback -> application-owned storage
```

Only the BTstack run-loop task accesses native PBAP, GOEP, OBEX, connection
state, or vCard parser state. Application callbacks continue to execute on the
xStack event task, never on the run-loop task.

## Upstream Source Integration

Import the required files from the exact upstream BTstack 1.6.2 tag. The
protocol implementation set is:

- `src/classic/pbap_client.c`;
- `src/classic/goep_client.c`;
- `src/classic/obex_message_builder.c`;
- `src/classic/obex_parser.c`;
- `src/classic/obex_iterator.c`;
- `src/classic/obex_srm_client.c`;
- `3rd-party/md5/md5.c`;
- `3rd-party/yxml/yxml.c`;
- their matching headers and license notices.

The implementation plan must derive the final source list from includes and a
clean link, rather than copying unrelated profiles. Existing upstream copyright
and license notices remain intact. The G+ project, generated makefiles, adapter
source manifest, packaging scripts, and exported header staging are updated
consistently.

## Components

### Generic PBAP profile wrapper

Add `btstack_pbap_client.c` and its public profile header. It owns:

- PBAP connection and operation state;
- the current peer address and PBAP CID;
- automatic category sequencing;
- PBAP packet flow-control calls;
- the vCard parser instance;
- one pending structured event;
- one bounded buffer for unconsumed bytes from the current PBAP data packet;
- native PBAP event translation into generic profile events.

It does not store a phone book or call-history database.

### Streaming vCard parser

Add a separately testable bounded parser invoked by the PBAP wrapper on the
run-loop task. It accepts arbitrarily split input chunks and emits a record only
after a complete `END:VCARD`.

The parser supports vCard 2.1 and 3.0 records, folded lines, UTF-8 data,
escaped values, and quoted-printable fields. It extracts:

- `FN`, falling back to `N` when necessary;
- up to four `TEL` values and their number types;
- `X-IRMC-CALL-DATETIME` and its incoming, outgoing, or missed qualifier.

Photo, address, email, organization, and other properties are ignored in this
release. Oversized text and excess phone numbers are truncated or omitted and
reported through a record truncation flag; they do not fail the synchronization.

### xStack API integration

Add `bt_xstack_phonebook.h` and include it from the public xStack aggregate
header. Extend `bt_xstack_config_t` with a phone-book callback.

The public commands are:

```c
bt_xstack_result_t bt_xstack_pbap_sync(
    const bt_xstack_addr_t *address);
bt_xstack_result_t bt_xstack_pbap_cancel(void);
bt_xstack_result_t bt_xstack_pbap_authenticate(
    const char *password);
bt_xstack_result_t bt_xstack_get_pbap_state(
    bt_xstack_pbap_state_t *state);
```

Like existing xStack commands, command return values report validation,
readiness, and queue admission only. Final protocol results arrive through
events.

The phone-book event types are:

- synchronization started;
- contact record;
- call-history record;
- category completed;
- synchronization completed;
- authentication required;
- error.

A contact contains a bounded name and up to four bounded phone numbers. A call
record contains a bounded name, number, timestamp, and direction. Every event
also identifies the peer and relevant category/status.

The complete phone-book event structure must not exceed the existing largest
xStack event payload. A compile-time assertion enforces this so adding PBAP does
not enlarge every existing event-queue slot.

## Synchronization State and Ordering

The normal automatic sequence is:

1. HFP service-level connection becomes active.
2. The event-side coordinator submits the same internal request used by
   `bt_xstack_pbap_sync()` with the HFP peer address.
3. PBAP connects to the phone PSE.
4. Pull `telecom/pb.vcf` for contacts.
5. Pull `telecom/ich.vcf` for incoming calls.
6. Pull `telecom/och.vcf` for outgoing calls.
7. Pull `telecom/mch.vcf` for missed calls.
8. Disconnect PBAP and emit the final synchronization result.

A category unsupported by the phone reports a category failure and does not
prevent later categories from running. Manual synchronization uses the same
state machine and is rejected as busy when another PBAP synchronization is
active.

## Reusing the Existing Event Queue

PBAP adds a phone-book domain to `bt_xstack_api_event_envelope_t` and posts
through the existing critical-event path. It does not create another queue,
worker task, or second event-copy stage.

Record events are non-droppable and obey a control-event reserve. With the
current 16-entry critical queue, PBAP record posting stops before consuming the
last four free entries. Those four entries remain available for connection,
HFP, A2DP, AVRCP, power, and PBAP lifecycle/error events. The reserve is a
compile-time constant and is covered by tests.

When a complete record cannot be posted without crossing the reserve:

1. the profile stores the complete record in its single pending-event slot;
2. parsing stops at the record boundary;
3. unconsumed bytes from the current PBAP packet are copied to the bounded
   replay buffer, whose capacity is at least the maximum PBAP body chunk
   accepted by the configured receive path;
4. `pbap_next_packet()` is not called;
5. after the event task frees sufficient queue space, it schedules one
   run-loop callback to post the pending record and resume parsing;
6. a new PBAP packet is requested only after pending and replayed data have
   been consumed.

The resume notification is edge-triggered: while PBAP is paused, only one
run-loop resume callback may be pending. This prevents notification storms and
unnecessary CPU wakeups.

The design adds only PBAP protocol/parser state, one pending record, and one
replay buffer. It does not add the roughly 3.5 KiB that another 16-entry xStack
event queue of the current payload size would require.

## Failure Handling

- A phone-book permission denial, PBAP connection failure, timeout, or fatal
  protocol error terminates the current synchronization and emits an explicit
  completion status.
- An unsupported or inaccessible individual category emits a failed category
  completion and the sequence continues.
- An OBEX authentication challenge emits `AUTHENTICATION_REQUIRED`. The
  application may answer with `bt_xstack_pbap_authenticate()`; an unanswered
  challenge times out and fails the synchronization.
- An HFP disconnect cancels the active PBAP operation. Complete records already
  accepted by the event queue are still delivered, followed by an aborted final
  event.
- A manual cancellation follows the same orderly abort and final-event path.
- Malformed fields are skipped when parser resynchronization at the next vCard
  boundary is possible. A malformed record increments a parser-error count and
  is not delivered as a partial contact.
- Queue pressure never drops a complete PBAP record. It pauses protocol input
  through PBAP flow control.
- PBAP failures do not change HFP, A2DP, or AVRCP connection state and do not
  mark the complete xStack service failed.

## Memory and CPU Policy

All PBAP state is statically bounded. No general-purpose heap allocation is
introduced. Buffer sizes and contact field limits are compile-time constants.

vCard parsing is incremental on the run-loop task. Each input byte is processed
once, and the parser does not scan or retain the complete phone book. Work per
run-loop callback is bounded by the current PBAP body chunk and stops early on
event backpressure. Application storage latency naturally slows PBAP transfer
through flow control without blocking the run-loop task.

Each delivered record incurs one copy into the existing xStack queue, one event
task dequeue, and one application callback. There is no second PBAP queue copy
or PBAP-specific task wakeup.

## Testing Strategy

### Source and build contracts

- Verify that every required PBAP/GOEP/OBEX/MD5/yxml source is present and
  compiled exactly once.
- Verify G+ project, makefile, package, and exported-header integration.
- Compile public headers as C and C++.
- Assert that the phone-book event does not increase the existing event
  envelope size.

### Parser tests

- vCard 2.1 and 3.0;
- input split at every possible byte boundary;
- folded lines and CRLF boundaries split across packets;
- UTF-8 Chinese names;
- multiple numbers and number types;
- quoted-printable and escaped values;
- incoming, outgoing, and missed call timestamps;
- overlong fields, excess numbers, ignored photo data, and malformed records.

### State-machine and queue tests

- automatic synchronization after HFP connection;
- manual synchronization, busy rejection, cancellation, and authentication;
- successful category ordering;
- unsupported categories and permission denial;
- HFP loss during each PBAP state;
- queue reserve enforcement;
- pending-record and replay-buffer resume;
- no record loss under a deliberately slow application callback;
- a single edge-triggered resume notification while paused;
- control events remain deliverable during phone-book synchronization.

### Target validation

Validate with representative Android and iPhone devices for permission grant
and denial, contacts with multiple numbers and Chinese text, call histories,
large phone books, reconnect, and manual resynchronization. Record:

- ROM and static RAM delta;
- maximum run-loop callback duration during parsing;
- PBAP synchronization duration and record count;
- event queue maximum depth and PBAP pause count;
- HFP call stability and A2DP/AVRCP behavior during synchronization.

## Delivery Boundary

The feature is complete when the exact-version protocol sources build, public
API and parser tests pass, no complete record is lost under queue pressure, the
existing event queue footprint does not grow, and board tests demonstrate
contact plus call-history synchronization without regressions to HFP, A2DP, or
AVRCP.

Application-side database design, contact deduplication, incremental database
change tracking, contact photos, address/email synchronization, PBAP Server
role, and UI work are outside this delivery.
