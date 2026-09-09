# BTstack AT Worker and Ring Design

## Goal

Make the build-time-selected AT frontend portable through the existing
BTstack platform boundary. Remove direct FreeRTOS task, queue, tick, and
20 ms polling dependencies from production code under `btstack/at` without
changing its public API, command syntax, response format, event format,
capacities, timeout, or API/AT build-time mutual exclusion.

## Scope

The AT and asynchronous API frontends remain separate build choices. They are
not linked or initialized together and their existing, incompatible
`bt_xstack_init` declarations remain unchanged.

Pure AT operation continues to use exactly two active BTstack-owned tasks after
Bluetooth is enabled:

1. `BTSTACK_WORKER_AT` for command parsing, completion handling, event output,
   and application output callbacks.
2. `BTSTACK_WORKER_STACK` for the BTstack Run Loop, HCI, timers, and profiles.

Before `AT+BTEN=1`, only the AT worker is running. The API and Event workers
are never started by the AT frontend.

## Non-goals

- Do not support simultaneous API and AT frontends in one firmware image.
- Do not change public AT names, signatures, commands, responses, or events.
- Do not add a task, dynamically allocated buffer, general queue abstraction,
  or heap allocation.
- Do not move UART receive/transmit ownership into BTstack.
- Do not change Stack Run Loop scheduling or profile behavior.
- Do not add bare-metal support in this change.

## Selected Architecture

Add an internal `BTSTACK_WORKER_AT` ID to the existing worker abstraction.
The GPCV2178 port owns its static stack, TCB, handle, and lifecycle state.
Other ports implement it through the same `validate_config`, `start`,
`notify`, `wait`, and `stop` operations used by the existing workers.

Replace the three AT-owned FreeRTOS queues with fixed typed storage and the
existing RTOS-neutral `btstack_ring`:

| Ring | Capacity | Element |
|---|---:|---|
| Input | 8 | `btstack_at_input_item_t` |
| Completion | 1 | `btstack_at_result_t` |
| Event | 16 | `btstack_at_event_t` |

The capacities remain identical to the existing AT queue configuration.
Ring check/push/pop operations execute inside the platform system critical
section. No worker waits and no application callback executes while that
critical section is held.

## Worker Signals

The AT frontend uses:

- `BTSTACK_WORKER_SIGNAL_WORK`: Input or Event Ring may contain data.
- `BTSTACK_WORKER_SIGNAL_COMPLETION`: Completion Ring may contain the result
  for the pending command.

Signals only wake the worker. The rings remain the source of truth, so signal
coalescing does not lose commands, completions, or events.

The existing API/Event signal meanings remain unchanged. The internal enum
and GPCV2178 worker table grow to include AT, but no additional worker is
started in an API-only build.

## Input Data Flow

`bt_xstack_send_at` continues to copy a complete command into internal storage.
It pushes the item to the Input Ring inside a short critical section, exits the
critical section, and notifies `BTSTACK_WORKER_AT` with `WORK`.

If the Input Ring is full, submission returns the existing
`BTSTACK_AT_STATUS_QUEUE_FULL` immediately. It never waits for space and never
calls an application callback in the submitting context.

## Command Dispatch and Completion

The AT worker parses and gates one command at a time, preserving current serial
command semantics. Queries that can be answered from a state snapshot and
commands dispatched through `bt_xstack_api_backend_dispatch` both publish a
`btstack_at_result_t` into the Completion Ring.

The completion producer pushes the result under the platform critical section,
then notifies the AT worker with `COMPLETION`. It never blocks the Stack Run
Loop. The AT worker matches both generation and command ID before accepting a
result.

The current `BTSTACK_AT_EXEC_TIMEOUT_MS` value of 1000 ms remains unchanged.
Expose a small `btstack_platform_time_ms()` forwarding wrapper over the already
required platform `system.time_ms` operation. The AT worker records an absolute
start time, recalculates the remaining timeout after stale completions, and
never extends the total deadline because of unrelated or old signals.

The Completion Ring keeps capacity one. A full completion ring returns the
existing queue-full status; no overwrite or blocking behavior is introduced.

## Asynchronous Event Data Flow

The Stack Run Loop maps a profile/core event to `btstack_at_event_t`, pushes it
to the Event Ring under a short critical section, exits the critical section,
and notifies the AT worker with `WORK`.

The Stack Run Loop never formats or outputs AT text and never calls the public
AT output callback. The AT worker drains and formats events after the current
command result, preserving the existing response-before-accumulated-events
ordering.

If the Event Ring is full, event posting remains non-blocking: increment the
existing overflow counter, return `BTSTACK_AT_STATUS_QUEUE_FULL`, and drop that
event. This preserves Run Loop isolation and the current observable behavior.

## AT Worker Loop

The AT worker follows this edge-triggered loop:

1. Pop at most one Input Ring item and process it serially.
2. Drain all currently queued asynchronous events through the output callback.
3. Repeat if input or event data is already present.
4. When both rings are empty, wait for `WORK` with
   `BTSTACK_WORKER_WAIT_FOREVER`.

While a command is pending, the worker waits only for `COMPLETION` using the
remaining command deadline. A `WORK` signal raised by an event remains pending;
the event itself remains in the Event Ring and is drained after command
completion or timeout.

The producer always pushes before notifying. If a notification arrives between
the consumer's empty check and wait, the platform notification bit remains
pending and the wait returns immediately. This prevents lost wakeups without
polling.

## Lifecycle and Rollback

`btstack_at_init` validates AT configuration without creating RTOS objects.
`btstack_at_start` initializes all three fixed rings, validates the AT worker
configuration, and starts `BTSTACK_WORKER_AT` through platform worker ops.

Initialization remains idempotent for identical configuration and rejects
conflicting configuration. Failed validation or worker start leaves the AT
frontend retryable and does not mark the common runtime permanently failed.
No partially started AT worker or initialized state is published on failure.

The existing Stack worker remains initialized through `btstack_runtime_init`
and is started only by the existing `AT+BTEN=1` gate.

## Platform Port Changes

The GPCV2178 FreeRTOS port adds one AT worker descriptor with static stack and
TCB storage. Its creation, notification, wait, and stop use the already audited
handle lifecycle and task-notification implementation. The AT frontend itself
must contain none of the following production dependencies:

```text
queue.h
task.h
QueueHandle_t
StaticQueue_t
TaskHandle_t
xQueueCreateStatic
xQueueSend
xQueueReceive
xTaskCreateStatic
xTaskGetTickCount
pdMS_TO_TICKS
```

## Build Boundary

`BTSTACK_COMMON_SOURCES`, API frontend, AT frontend, Legacy AT core, and the
selected port remain separate source sets. The internal worker enum change and
platform time wrapper are common; `btstack_at.c` remains in the AT frontend.

API, AT, and Legacy archives must each contain exactly one selected platform
descriptor. The AT archive must contain `btstack_ring` and the GPCV2178 AT
worker implementation exactly once. No API frontend source enters the AT or
Legacy frontend sets.

## Verification

Use TDD with real production `btstack_ring` and platform worker boundaries.
Required behavioral coverage:

- AT start validates and starts only `BTSTACK_WORKER_AT`.
- Pure AT initialization does not start API or Event workers.
- `AT+BTEN=1` starts the existing Stack worker, giving two active workers.
- Input submission wakes AT and preserves FIFO order and queue-full status.
- Synchronous and asynchronous completions wake AT and match generation/ID.
- Stale completion does not extend the one-second deadline.
- Event posting wakes AT, preserves FIFO order, and executes output callbacks
  only in AT worker context.
- Slow output cannot block the Stack Run Loop; full Event Ring drops and counts
  overflow exactly as before.
- No 20 ms polling or FreeRTOS task/queue token remains in AT production code.
- Worker start failure is retryable and leaves no partially published state.
- Existing AT command/response/event contracts remain byte-compatible.
- Existing API worker/ring tests remain green after the internal worker enum
  grows.
- API, AT, Legacy, and main firmware fresh builds succeed with no new warning
  attributable to this change.

## Acceptance Criteria

The work is complete when the AT frontend has zero direct RTOS dependencies,
uses fixed rings plus worker notification with no periodic polling, maintains
its existing public and wire behavior, keeps the pure AT task count at two after
Bluetooth enable, and passes all focused and full build/test verification.
