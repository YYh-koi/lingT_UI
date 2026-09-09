# xStack API Run Loop Boundary Design

## Goal

Reduce BTstack run loop occupancy caused by application API requests while
preserving the existing asynchronous `bt_xstack_*` command contract,
synchronous cached Getters, and callback-only state reporting.

## Current Behavior

Public setters and controls already validate and copy their arguments into a
static request queue. The API worker receives one request at a time, but then
schedules the complete backend request handler onto the BTstack run loop. That
handler currently performs API-state reads and writes, address resolution,
name formatting, control selection, event creation, BTstack calls, and an
unconditional GAP state synchronization.

Synchronous Getters such as `bt_xstack_get_avrcp_play_state()` already read the
protected `bt_xstack_api_state` snapshot directly and do not enter the run
loop. This behavior remains unchanged.

## Considered Approaches

1. Keep the complete request handler on the run loop. This preserves behavior
   but retains unnecessary run loop work.
2. Prepare a minimal stack operation in the API worker and schedule only that
   operation on the run loop. This is the selected approach because it reduces
   run loop work without changing the public API or BTstack run-loop model.
3. Add a new command queue directly to the run loop implementation. This could
   remove one callback layer, but couples the public API backend to the port and
   is unnecessary for the current request volume.

## Architecture

The request path is split into three responsibilities:

1. The public API validates arguments and copies an immutable request into the
   existing request queue.
2. The API worker performs request preparation. It resolves cached peer
   addresses, formats the final local name, selects explicit HFP/AVRCP control
   operations, updates pure API state, and packages a minimal prepared request.
3. The run-loop callback performs only operations that access BTstack-owned
   state through `btstack_gap_*`, `btstack_hfp_hf_*`, `btstack_a2dp_sink_*`, or
   `btstack_avrcp_*` APIs, then posts the existing internal completion token.

The boundary is based on whether an operation touches BTstack-owned state, not
whether its implementation file is physically under `src/`.

## Pure API Requests

The following requests complete in the API worker and never schedule a run
loop callback:

- profile mask configuration;
- peer cache clear by index, address, or all peers;
- trusted-device-list configuration stored by the API state layer;
- HFP API configuration;
- microphone mute state/event used by the API facade;
- A2DP mute state stored by the API facade.

These operations use the existing critical-section-protected state helpers.
They still post the existing internal completion token so the serialized API
worker flow is unchanged.

## Prepared Stack Requests

Before scheduling the run loop, the API worker prepares all data that does not
require live BTstack state:

- local-name suffix expansion is completed into the request text buffer;
- connect-last resolves the cached peer and produces an explicit address;
- native address bytes are copied into the prepared request;
- play/pause is resolved to an explicit play or pause control using the cached
  play state;
- HFP toggle-audio is resolved to explicit connect or disconnect audio using
  the cached HFP audio state;
- scan filters are copied into API event state before the stack scan starts.

Connection checks that require BTstack-owned live state stay in the run-loop
operation. They are intentionally small and prevent invalid stack calls.

## State Synchronization

The unconditional `bt_xstack_api_state_sync_gap()` after every request is
removed. GAP synchronization runs only after successful operations that can
change the cached GAP configuration or scanning state. Profile, phone, A2DP,
and AVRCP runtime state continues to be updated from BTstack events.

Pure API state changes are applied by the API worker. Stack-backed settings
that are represented in the public snapshot are updated only after the stack
operation reports success, using the existing protected state helper.

## Concurrency and Lifetime

The existing API worker serializes requests and waits for one internal
completion before taking the next request. Therefore one static prepared
request and one static run-loop callback registration remain sufficient. All
strings and addresses are copied before scheduling, so the run loop never
references application-owned memory.

No public command-completion callback is introduced. Public setters continue
to report only argument, initialization, and queue-admission results. The
application determines final state from subsequent state events or synchronous
Getters.

## Error Handling

Preparation failures complete internally without entering the run loop. A
failed BTstack operation also completes internally so the API worker cannot
stall. Existing state events remain the source of truth; failed stack
operations do not optimistically update stack-backed cached fields.

## Verification

Contract tests must prove that:

- all public Getters remain snapshot-only and do not dispatch to the run loop;
- pure API requests are handled outside the run-loop request switch;
- name, address, and toggle-control preparation occurs before
  `btstack_run_loop_execute_on_main_thread()`;
- the run-loop callback contains only the prepared stack-operation execution
  and internal completion;
- unconditional GAP synchronization is absent;
- existing API surface and AT-parity contract tests still pass;
- the independent API library and Debug application both build successfully.
