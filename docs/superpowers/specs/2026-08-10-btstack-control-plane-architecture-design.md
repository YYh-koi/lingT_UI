# BTstack Control-Plane Architecture Optimization Design

## Objective

Optimize the xStack control plane without changing the public asynchronous
`bt_xstack_*` command semantics:

- public commands report only admission/enqueue status;
- no `COMMAND_COMPLETE` or public command-completion callback is introduced;
- applications continue to determine final state from later domain events and
  synchronous cached-state Getters;
- only code that owns or calls BTstack state executes on the BTstack run-loop
  task;
- application callbacks never execute on the BTstack run-loop task.

This specification covers the first of three independent stages. A2DP decode
offload and persistent bonding storage will be designed and delivered in later
stages after this control-plane stage passes regression and board validation.

## Current Constraints

The implementation currently has four structural constraints:

1. The platform `execute_on_main_thread` implementation modifies the BTstack
   base callback linked list from tasks other than the run-loop task while the
   run-loop task pops the same list.
2. The API task processes one request and waits for an internal completion
   before it drains events. A one-second internal timeout permanently marks the
   whole API service failed.
3. Event posting uses a single fixed queue. Posting does not immediately wake
   a task blocked on the request queue, and all event classes share the same
   loss policy.
4. Individual Getters copy the complete `bt_xstack_state_t` under a critical
   section even when they return only one byte.

The current static allocation policy for API requests and events must be
preserved. This stage must not introduce general-purpose heap allocation.

## Considered Approaches

### Approach A: Add critical sections around the existing callback list

Protect `btstack_run_loop_base_add_callback()` and the corresponding pop with
FreeRTOS critical sections, but otherwise retain the single pending request and
completion wait.

This is the smallest change, but it leaves event latency, global failure after
one timeout, and the single-pending-request design intact. It also places
platform concurrency policy inside generic BTstack code.

### Approach B: Port-owned ingress ring and split API workers (selected)

Add a fixed-capacity, port-owned callback ingress ring. Producers enqueue
callback registrations under a short critical section and wake the run loop.
Only the run-loop task removes and invokes registrations. Split command
admission from event delivery, and use a fixed command-slot pool for prepared
BTstack commands.

This keeps concurrency policy in the GPCV2178 port, removes cross-task linked
list mutation, preserves FIFO ordering, and creates clean measurement points.
It requires moderate changes but does not alter the public API.

### Approach C: One new dispatcher task owns all control-plane work

Move requests, events, and stack dispatch into a new dispatcher task and let
the BTstack run loop call back into it synchronously.

This creates another ownership boundary without removing the need to marshal
BTstack calls onto the run loop. It also makes the dispatcher a new bottleneck,
so it is not selected.

## Selected Architecture

```text
Application tasks
    |
    | bt_xstack_* command (validate and copy)
    v
API request queue --> API command task --> prepared command slot
                                             |
                                             v
                                GPCV2178 run-loop ingress ring
                                             |
                                             v
                                    BTstack run-loop task
                                             |
                                  stack event translators
                                             |
                     +-----------------------+------------------+
                     |                                          |
              critical event queue                    best-effort event queue
                     |                                          |
                     +-----------------------+------------------+
                                             |
                                      API event task
                                             |
                                  application callbacks

Cached state store <---- stack event translators / successful local changes
       |
       +---- synchronous field/domain Getters (never enter run loop)
```

## Component Design

### 1. GPCV2178 run-loop ingress ring

`btstack_run_loop_gpcv2178.c` will own a fixed ring of
`btstack_context_callback_registration_t *` entries.

- Capacity is compile-time fixed and at least the API command-slot count plus
  existing BTstack internal main-thread callbacks.
- Enqueue performs duplicate-pointer suppression to preserve the current
  linked-list behavior where the same registration cannot be queued twice.
- Enqueue, dequeue, indices, count, and maximum depth are protected by a short
  FreeRTOS critical section.
- `execute_on_main_thread` only enqueues and wakes the run loop. It never calls
  `btstack_run_loop_base_add_callback` from a foreign task.
- The run-loop task drains the ingress ring before timers and data sources. It
  invokes each callback outside the critical section.
- Queue-full is counted. The port exposes a read-only diagnostics snapshot to
  the API diagnostics aggregator.
- The generic BTstack base callback list remains available for operations that
  are already wholly owned by the run-loop task, but the GPCV2178 cross-task
  entry no longer mutates it.

### 2. Prepared command-slot pool

The API backend's single static pending request is replaced by a fixed array of
command slots. Each slot contains:

- an immutable prepared `bt_xstack_api_request_t`;
- its own BTstack callback registration;
- a state flag (`FREE`, `QUEUED`, or `RUNNING`).

The API command task validates and prepares a request before acquiring a slot.
Pure API-local requests continue to execute in the API command task and never
use a slot. A stack-backed request is copied into a free slot and its callback
is submitted to the run-loop ingress ring. The run-loop callback changes the
slot to `RUNNING`, performs the minimal stack operation, records an internal
result counter, releases the slot, and notifies the API command task.

There is no public completion and no completion queue. If all slots are busy,
the API command task waits for a slot notification while already-admitted
requests remain safely stored in the request queue. Slot count is sized to the
request queue so an accepted request is not silently discarded.

FIFO admission and run-loop execution ordering are preserved.

### 3. Separate event task and event classes

Control events are separated from best-effort discovery results.

- Critical queue: service lifecycle, power, connection, phone, HFP audio,
  A2DP state, AVRCP state, and scan completion events.
- Best-effort queue: inquiry/scan result events only.
- Every successful post notifies the API event task immediately; fixed 20 ms
  polling is removed.
- The event task always drains the critical queue first, then processes a
  bounded number of best-effort entries before checking the critical queue
  again.
- Application service/connection/phone/audio-event callbacks execute only in
  the API event task.
- Media and call PCM are explicitly outside this control-event mechanism and
  remain unchanged in this stage. Their migration is the second-stage audio
  design.
- Critical and best-effort drop counts are maintained separately. No command
  completion notification is synthesized when a stack command fails.

### 4. Recoverable control-plane errors

A failed or rejected individual stack operation increments diagnostics and
releases its command slot. It does not set the global API service to failed.

Global failed state is reserved for initialization failure or loss of the
underlying run-loop/transport service. The API command task remains able to
process subsequent requests after one command failure.

The public API continues to return immediate argument, initialization, ready,
and request-queue admission errors. Preconditions that can be determined from
cached state are checked before enqueue where doing so cannot race with a valid
later state transition. Final protocol results remain event/state based.

### 5. Domain state access

The state owner remains the API event/state module, but it provides short,
typed readers for frequently queried fields:

- power and device flags;
- HFP state, audio state, mute, and speaker volume;
- A2DP state and mute;
- AVRCP connection, play state, configuration, repeat, shuffle, and absolute
  volume;
- peer count and indexed peer copy.

Individual public Getters use these readers and do not copy the complete state.
Each typed reader protects only its field or small domain copy. The existing
full-state Getter retains a consistent whole-state snapshot for callers that
explicitly request it.

No Getter calls BTstack or waits on the run loop.

### 6. Diagnostics

Add a synchronous `bt_xstack_get_diagnostics()` Getter backed only by cached
counters. It returns at least:

- accepted and rejected API requests;
- stack command executions and stack command failures;
- current and maximum command-slot usage;
- run-loop ingress full count and maximum depth;
- critical event drops and best-effort event drops.

Counters are monotonic `uint32_t` values. Counter wrap is permitted. Reading
diagnostics uses a short critical section and does not enter the run loop.

This is an observability Getter, not a command completion channel.

## Public Compatibility

- Existing headers and functions retain their names and behavior.
- No AT string or parser is reintroduced.
- No `bt_xstack_send_at()` symbol is introduced.
- No `COMMAND_COMPLETE` event or public per-command completion callback is
  introduced.
- One additive public type and Getter for diagnostics is allowed.
- Existing service, connection, phone, audio event, media PCM, and call PCM
  callback signatures remain unchanged.

## Failure and Overflow Policy

- Public request queue full: return `BT_XSTACK_QUEUE_FULL` immediately.
- Command slots temporarily full: wait internally for a released slot; do not
  discard an already accepted request.
- Run-loop ingress unexpectedly full: retain the command slot and retry after
  notification/yield; count the occurrence. Do not overwrite an older entry.
- Critical event queue full: increment critical-drop diagnostics and ensure the
  cached state is still updated so synchronous Getters remain authoritative.
- Best-effort event queue full: drop the newest scan result and increment the
  best-effort-drop counter.
- Individual stack operation failure: count and continue processing later
  commands.
- Initialization or transport failure: retain the existing global not-ready
  behavior.

## Testing Strategy

Tests are added before each production change and must demonstrate the old
architecture failing the new contract.

1. Run-loop ingress contract verifies that the GPCV2178 external entry uses a
   port-owned fixed ring and does not call
   `btstack_run_loop_base_add_callback` from `execute_on_main_thread`.
2. Command-pool contract verifies removal of the single pending request and
   completion queue, per-slot callback registrations, slot release, and no
   public completion event.
3. Event-task contract verifies separate critical/best-effort queues,
   notification-driven delivery, and absence of 20 ms polling.
4. State-reader contract verifies frequently used Getters call typed cached
   readers and never submit requests or copy the full state.
5. Recovery contract verifies individual stack command errors do not mark the
   complete API failed.
6. Diagnostics header/build contracts verify C and C++ compatibility and the
   absence of AT entry points.
7. Existing API, Demo, SCO, port, transport, AVRCP, library build, and Debug
   application regressions remain green.

Board validation after host/build verification measures command/event latency,
run-loop maximum busy time, event drops, A2DP playback stability, SCO playback,
and reconnect behavior. CPU reduction is not claimed until measured on target.

## Delivery Boundary

This stage is complete when the control-plane tests pass, the API library and
Debug Demo link successfully, symbol checks pass, and the current external API
semantics are preserved. A2DP SBC decode still executes in the run loop until
the separately reviewed second-stage audio design is implemented.
