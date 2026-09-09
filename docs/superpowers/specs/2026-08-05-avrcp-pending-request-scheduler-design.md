# AVRCP Pending Request Scheduler Design

## Goal

Prevent AVRCP metadata and periodic play-status requests from being lost when
the AVRCP Controller already has one command in flight. Preserve the existing
single-command `avrcp_controller.c` state machine and avoid a general-purpose
message queue.

## Scope

- Schedule two idempotent query types: current-track metadata and play status.
- Coalesce repeated requests of the same type with a pending bitmask.
- Give metadata higher priority than periodic play-status polling.
- Keep `AVRCPCFG` bit 0 as the switch controlling automatic metadata requests.
- Keep ordered or parameterized control commands such as play, pause, next,
  previous, and absolute volume on their current path. A control-command FIFO
  is not part of this change.
- Do not change public AT event formats or enable HCI/UART diagnostic dumps.

## Architecture

`avrcp_controller.c` remains responsible for the single active AVRCP command.
`btstack_avrcp.c` gains a lightweight pending scheduler containing:

- A pending bitmask with `METADATA` and `PLAY_STATUS` bits.
- A short retry timer used only while at least one request is pending.
- A service function that attempts at most one request per invocation.

The scheduler does not copy commands or allocate queue nodes. Repeated metadata
or play-status requests set an already-set bit and are therefore coalesced.

## Request Flow

1. AVRCP connection or `TRACK_CHANGED` causes the AT backend to request
   metadata when `AVRCPCFG` bit 0 is enabled.
2. The profile clears the current metadata fields, sets `METADATA` pending, and
   arms the retry timer instead of submitting from inside the event callback.
3. The periodic progress timer sets `PLAY_STATUS` pending rather than calling
   `avrcp_controller_get_play_status()` directly.
4. The retry handler services `METADATA` first. A successful Controller API
   return clears that bit because the command is now owned by the Controller.
5. `ERROR_CODE_COMMAND_DISALLOWED` leaves the bit pending and rearms the retry
   timer. This covers an active command, initialization capability exchange,
   notification registration, response wait, and response timeout recovery.
6. If another bit remains after a successful submission, the retry timer stays
   armed and submits it after the Controller becomes idle.
7. Disconnect cancels the retry timer and clears all pending bits.

## Metadata Completion

The existing response path remains unchanged:

- `NOW_PLAYING_TITLE_INFO`, `ARTIST_INFO`, and `ALBUM_INFO` fill the profile
  buffers.
- Successful `NOW_PLAYING_INFO_DONE` emits `BTSTACK_AVRCP_EVENT_TRACK_INFO`.
- The AT backend publishes the existing `+TRACKINFO` event using `0x0c` field
  separators.

Rejected or timed-out Controller commands do not block later work. A request
that was never accepted remains pending; a request accepted by the Controller
is considered consumed and follows the Controller's existing timeout behavior.

## Error Handling

- Unknown/disconnected AVRCP CID clears pending work during disconnect cleanup.
- Busy/command-disallowed is retryable and produces no high-frequency log.
- Invalid parameters or other non-busy submission failures clear the affected
  request to avoid an infinite retry loop.
- Existing Controller response timeout and disconnect behavior are preserved.

## Verification

- Add a focused failing source-contract test for the bitmask, priority order,
  deferred submission, busy retry, coalescing, and disconnect cleanup.
- Run existing AVRCP/AT protocol tests.
- Rebuild `libxstack.a`, the main ELF, and `project.bin`.
- Confirm the final ELF retains `+TRACKINFO` support and does not restore UART
  positioning or full HCI dump strings.
- Board validation: connect a phone, start music, and confirm one
  `+TRACKINFO` event after connection/current-track availability and after a
  track change, while `+PLAYSTAT`/`+TRACKSTAT` continue normally.
