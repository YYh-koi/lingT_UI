# Application-Owned Phonebook Synchronization Design

## Objective

Make phone-book synchronization an explicit application action instead of an
automatic protocol-stack action. Expose application-facing names in the
`phonebook` domain, while retaining PBAP, GOEP, OBEX, SRM, and other Bluetooth
protocol terminology inside the stack.

This design supersedes the automatic HFP-trigger and public PBAP naming in
`2026-08-11-btstack-pbap-client-design.md`. The existing parser, generic event
queue, flow control, bounded memory policy, and record delivery behavior remain
unchanged.

## Selected Approach

Use a category bit mask in one asynchronous synchronization request. A caller
may select one category, any combination, or all categories. The wrapper opens
one PBAP connection, pulls only the selected categories in a fixed order, then
closes the PBAP connection.

This is preferred over a single-value enum because common sequences such as
contacts plus missed calls can use one PBAP connection and cannot race with a
second asynchronous request. It is preferred over an options structure because
the current operation has only one independent option and does not need the
extra API surface.

## Public Application API

The public header uses phone-book terminology only:

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

Public state types, state constants, and password-size constants are renamed
from `PBAP` to `PHONEBOOK` consistently. Existing phone-book callback, event,
contact, call, direction, status, and singular event-category names remain in
the phone-book domain. The event `category` field continues to identify one
actual category; it is not a mask and never reports `ALL`.

The old public `bt_xstack_pbap_*`, `bt_xstack_get_pbap_state`,
`bt_xstack_pbap_state_t`, and `BT_XSTACK_PBAP_*` identifiers are removed rather
than retained as aliases. The API has not been released as a compatibility
contract, and exposing both vocabularies would make the application boundary
less clear.

## Mask Validation

`bt_xstack_phonebook_sync()` rejects the request as an invalid argument when:

- `address` is null;
- `categories` is zero; or
- `categories` contains a bit outside `BT_XSTACK_PHONEBOOK_ALL`.

A valid mask is copied by value into the existing xStack request object's
`values[0]` slot; the request structure and queue-slot size do not grow. The
application may reuse or release its input variables as soon as the API
returns.

## Application-Owned Trigger

The xStack HFP event coordinator no longer creates a PBAP synchronization
request when it receives `BT_XSTACK_PHONE_EVENT_DEVICE`. It only delivers the
phone event to the registered application callback.

The demo becomes the example policy owner:

```c
if (event->id == BT_XSTACK_PHONE_EVENT_DEVICE) {
    bt_xstack_result_t result = bt_xstack_phonebook_sync(
        &event->address, BT_XSTACK_PHONEBOOK_ALL);
    DBG_PRINT("xstack demo phonebook sync status=%d\r\n", result);
}
```

Other applications may request a smaller mask, defer synchronization, or not
synchronize at all. The call is asynchronous: its return value reports input
validation and request-queue admission; records and final protocol status are
reported through the phone-book callback.

HFP `LOST` handling remains an internal stack responsibility. It reliably
cancels or cleans up an active PBAP operation for that peer without adding a
new task or queue.

## Internal Protocol Naming

The implementation boundary retains professional protocol names:

- internal requests remain `BT_XSTACK_REQUEST_PBAP_*`;
- the backend and generic profile remain `btstack_pbap_client_*`;
- native BTstack calls remain `pbap_*`;
- GOEP, OBEX, SRM, yxml, and MD5 identifiers remain unchanged.

The internal PBAP sync request gains the selected category mask. This is an
implementation detail and is not exposed as a native BTstack type to the
application.

## Pull Sequence and Results

For each valid request, the generic PBAP wrapper:

1. opens one PBAP connection to the requested peer;
2. examines categories in contacts, incoming, outgoing, missed order;
3. pulls each selected category and skips every unselected category;
4. emits each parsed vCard through the existing generic xStack event queue;
5. emits one `CATEGORY_COMPLETED` event for each selected category;
6. disconnects PBAP after the last selected category; and
7. returns to `IDLE` before emitting the final `COMPLETED` event.

An unsupported or inaccessible selected category reports its category result
and does not prevent later selected categories from running. The final result
is `SUCCESS` when every selected category succeeds and `PARTIAL` when at least
one selected category fails but the overall transaction can continue. Fatal
connection, authentication, cancellation, or protocol errors retain their
existing terminal statuses.

Only the PBAP/GOEP/OBEX session is disconnected. The Bluetooth ACL, HFP, A2DP,
and AVRCP connections are not disconnected by normal phone-book completion.

## Completion and Chained Requests

`CATEGORY_COMPLETED` means that one category has finished, but PBAP may still
be pulling another category or closing the session. Applications must not use
it as the readiness signal for another synchronization.

`COMPLETED` is emitted only after native PBAP cleanup has finished and the
wrapper plus cached public state are both `IDLE`. Therefore an application may
reliably call `bt_xstack_phonebook_sync()` from the `COMPLETED` callback to
start another request. Calling sync while a request is active retains the
existing asynchronous `BUSY` result behavior and does not disturb the active
peer or transfer.

## Queue, Memory, and CPU Policy

This change adds no task and no event queue. Phone-book records continue to use
the existing generic critical-event queue, reserve, pending-record slot, replay
buffer, and edge-triggered resume mechanism.

The PBAP wrapper stores one byte of category mask. The xStack request carries it
in an existing value slot, so this change does not enlarge request-queue items.
Mask validation and four fixed-order selection checks are negligible CPU work.
Selecting several categories in one request reduces connection setup and
teardown work compared with multiple single-category synchronizations.

## Testing and Delivery

Implementation follows test-driven development. Focused contracts and state
machine tests must prove:

- the public header exports only the new phone-book names and mask API;
- C and C++ callers can compile all single-category, combined, and `ALL` uses;
- zero and unknown category bits are rejected;
- a subset pulls exactly the selected categories in fixed order;
- `ALL` pulls all four categories through one PBAP connection;
- unsupported selected categories continue to later selected categories;
- HFP `DEVICE` does not submit an internal PBAP request;
- the demo calls `bt_xstack_phonebook_sync(..., BT_XSTACK_PHONEBOOK_ALL)` after
  receiving HFP `DEVICE` and logs queue-admission failure;
- HFP `LOST` still performs reliable internal PBAP cleanup;
- final `COMPLETED` is delivered after `IDLE`, and a sync submitted from that
  callback succeeds;
- busy rejection does not alter the active request;
- no PBAP-specific queue or task is introduced; and
- generated G+ project files, the ARM archive, staged public headers, README,
  demo contract, API contracts, parser tests, and PBAP profile tests pass after
  a fresh rebuild.

Target verification should confirm that the demo log shows the HFP `DEVICE`
event, the application sync-admission result, selected record/category events,
and one final completion. Existing Android/iPhone permission, authentication,
large-phone-book, reconnect, and queue-pressure checks remain applicable.

## Out of Scope

This change does not add contact storage, deduplication, incremental PBAP
change tracking, SIM phone-book selection, arbitrary PBAP repository paths,
contact photos, a PBAP Server role, or another event queue/task.
