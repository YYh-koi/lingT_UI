# BTstack Cross-RTOS Platform Port Design

## Goal

Make the BTstack common layer independent of FreeRTOS, GPCV2178, and vendor
SDK headers so that a new RTOS platform is added by implementing and selecting
one `ports/<platform>` package. Preserve all existing public APIs, event and
queue behavior, asynchronous execution contexts, memory limits, and GPCV2178
runtime behavior. Bare-metal operation is outside this design.

## Compatibility Contract

The refactor must not change:

- Existing `bt_xstack_*` function signatures.
- Published enum values or event structure layouts.
- API request, critical-event, or best-effort-event queue capacities.
- The four-slot control-event reserve, FIFO rules, PBAP backpressure, drop
  counters, or reliable HFP-lost cleanup.
- The API worker, event worker, and BTstack worker execution model.
- Ordinary application callbacks running on the event worker.
- A2DP and HFP data callbacks running directly on the BTstack worker.
- GPCV2178 task priorities, stack sizes, initialization behavior, and supported
  features.

The only public API addition is:

```c
bt_xstack_result_t bt_xstack_call_audio_write(
    const uint8_t *data,
    uint32_t length);
```

Applications that do not call this function retain their current behavior.

## Architecture

The common layer owns protocol and business behavior. A single platform
descriptor supplies RTOS scheduling and hardware bindings.

```text
Application
  |
  | bt_xstack_* public API
  v
Common core
  |-- static request and event rings
  |-- queue priority, reserve, and backpressure rules
  |-- API/event worker processing functions
  |-- BTstack, profiles, PBAP, and event mapping
  |-- platform-independent initialization order
  `-- btstack_platform_t
          |
          v
ports/<platform>
  |-- RTOS task, notify, and wait implementation
  |-- critical section, time, delay, and logging
  |-- BTstack run loop
  |-- HCI UART/USB/SPI transport and controller control
  |-- persistent storage and controller-address services
  `-- platform configuration
```

The common layer keeps its fixed-capacity rings. A port does not implement or
own business queues; it only creates, waits, and wakes execution contexts. This
keeps memory use, queue ordering, and backpressure identical on every RTOS.

## Platform Selection

Each build links exactly one platform package, which exports:

```c
const btstack_platform_t *btstack_platform_default(void);
```

`bt_xstack_init()` obtains the descriptor through this internal symbol, so the
public initialization API does not gain a platform parameter. Linking no port
causes an unresolved symbol. Linking multiple ports causes a duplicate-symbol
link error.

The platform descriptor begins with version and size fields:

```c
typedef struct {
    uint16_t abi_version;
    uint16_t struct_size;
    const char *name;
    btstack_system_ops_t system;
    btstack_worker_ops_t workers;
    btstack_storage_ops_t storage;
    btstack_controller_ops_t controller;
    btstack_stack_ops_t stack;
} btstack_platform_t;
```

The common layer validates `abi_version` and `struct_size` before reading any
later member. The descriptor, its name, nested ops tables, and every callback
context must be static or application-owned and remain valid for the entire
process or firmware lifetime after successful activation. `late_deinit()`
releases only running resources created by `early_init()`; it must not release
the descriptor, its nested ops, or any callback context.

## Ops Responsibilities

### System Ops

System ops provide only foundational RTOS-neutral services:

```c
typedef struct {
    void *context;
    void (*enter_critical)(void *context);
    void (*exit_critical)(void *context);
    uint32_t (*time_ms)(void *context);
    void (*delay_ms)(void *context, uint32_t delay_ms);
    void (*log)(void *context,
                btstack_log_level_t level,
                const char *tag,
                const char *message);
} btstack_system_ops_t;
```

`enter_critical` and `exit_critical` are mandatory and must be supplied as a
pair. `time_ms` is mandatory, monotonic modulo unsigned wraparound, and uses
milliseconds. Delay and logging are optional.

Common sources must not directly call RTOS critical-section, tick, delay, or
vendor print functions.

### Worker Ops

The common layer recognizes three execution contexts:

```c
typedef enum {
    BTSTACK_WORKER_API = 0,
    BTSTACK_WORKER_EVENT,
    BTSTACK_WORKER_STACK
} btstack_worker_id_t;
```

Worker ops provide these logical operations:

- Start a worker with an entry function and argument.
- Notify a worker with signal bits.
- Wait for selected signal bits with a millisecond timeout.
- Stop and join a worker during initialization rollback or internal test
  teardown.

Task handles, native notification objects, stack storage, task priorities, and
RTOS-specific timeout conversion remain private to the port. Notify must be
safe from all normal producer contexts used by the current stack. ISR-specific
entry points are not added until a concrete producer requires them.

### Storage Ops

Storage keeps the existing key/tag and bounded-buffer semantics:

- Read a value and return its actual bounded length.
- Write a complete value.
- Delete a value.

Storage is optional. Without it, volatile properties remain in RAM. Internal
property operations return `BTSTACK_STATUS_NOT_SUPPORTED`, and existing public
APIs retain their current result-code mapping; this refactor does not add a
public `BT_XSTACK_NOT_SUPPORTED` enumerator.

### Controller Ops

Controller ops provide controller power/reset and optional address services:

- Power on, power off, and reset the controller.
- Apply a public or random controller address.
- Obtain a random static address when supported.

Power and reset are mandatory. Address services are optional.

### Stack Ops

Stack ops bind the common host stack to one platform:

- Return the `btstack_run_loop_t` instance.
- Return the `hci_transport_t`, its immutable configuration, and
  `btstack_control_t`.
- Post a callback into the BTstack run-loop context.
- Perform platform early initialization and late deinitialization.

The common layer owns the generic BTstack initialization sequence, including
memory, HCI, L2CAP, SDP, profiles, and the API bridge. GPIO, clocks, DMA,
controller firmware, physical UART/USB/SPI, and native RTOS setup stay in the
port.

## Queue and Worker Data Flow

```text
bt_xstack_* caller
  -> copy bounded arguments into common request ring
  -> leave common critical section
  -> notify API worker
  -> API worker maps request to an internal command
  -> stack.post_callback()
  -> profile and BTstack state machines run on stack worker
  -> copy ordinary result into common event ring
  -> leave common critical section
  -> notify event worker
  -> event worker invokes the application callback
```

Required invariants:

- Application event callbacks never run from a `bt_xstack_*` call stack or an
  HCI ISR.
- Queue capacity checks and writes occur in one common critical section.
- Worker notification occurs after leaving the critical section.
- The existing control-event reserve and PBAP record backpressure rules remain
  common-core behavior.
- Releasing event space resumes PBAP through the existing generic request
  path, not through a platform queue.
- No new general-purpose business queue or task is introduced.

## Audio Data Boundary

Audio rendering and capture do not belong to `btstack_platform_t`. The port
only transports HCI/SCO packets and controls the Bluetooth controller. DAC,
I2S, playback, recording, volume processing, and application audio buffering
belong to the application.

### Downlink

Existing callbacks remain the public data plane:

- `bt_xstack_media_audio_callback_t` receives decoded A2DP PCM.
- `bt_xstack_call_audio_callback_t` receives HFP call audio.

Audio callbacks run directly on the BTstack worker to avoid copying high-rate
payloads through the ordinary event queue. Their data pointer is valid only
during the callback, and the callback must return promptly. Applications that
need deferred processing copy the data into an application-owned audio buffer.
An absent callback discards the payload and increments a diagnostic counter
without blocking the protocol stack.

A2DP format changes continue to use
`BT_XSTACK_AUDIO_EVENT_PCM_CONFIG`. Existing HFP state/configuration events
continue to describe call-audio state.

### Uplink

`bt_xstack_call_audio_write()` accepts application microphone audio. The
common layer copies accepted data into a statically allocated, fixed-capacity
HFP TX ring. The BTstack worker drains that ring only when the SCO link reports
can-send-now.

The call is non-blocking:

- Null data or zero length returns `BT_XSTACK_INVALID_ARGUMENT`.
- No active writable call-audio connection returns `BT_XSTACK_NOT_READY`.
- Insufficient TX capacity returns `BT_XSTACK_QUEUE_FULL` without partial
  acceptance.
- Accepted data returns `BT_XSTACK_OK` and remains owned by the common ring.

The TX ring is separate from the general request/event rings, uses no dynamic
allocation, and has these fixed limits:

```c
#define BT_XSTACK_CALL_AUDIO_TX_BYTES      2048u
#define BT_XSTACK_CALL_AUDIO_WRITE_MAX      255u
```

An input longer than `BT_XSTACK_CALL_AUDIO_WRITE_MAX` returns
`BT_XSTACK_INVALID_ARGUMENT`. The worker splits accepted ring data according
to the active controller SCO payload length and never emits a partial SCO
frame solely to empty the ring.

## Transport Boundary

`btstack/transport` contains only reusable protocol and transport interfaces:

- H4 and H5 framing/state machines.
- SLIP encoding and decoding.
- Abstract UART, UART-block, USB, and SPI contracts.

It must not include RTOS, chip, board, or vendor SDK headers.

Required cleanup:

- Remove `gplib_print_string.h` from H4/H5 and use BTstack's generic logging.
- Move Windows, FreeRTOS, and embedded instance declarations out of
  `btstack_uart_block.h` and into their owning ports.
- Keep `btstack_config.h` feature consumption, while making the selected
  platform configuration the port's responsibility.

## Directory Layout

```text
btstack/
|-- api/                         public bt_xstack implementation
|-- core/                        platform-independent runtime and queues
|   |-- btstack_platform.c
|   |-- btstack_worker.c
|   |-- btstack_runtime.c
|   |-- btstack_core_backend.c
|   `-- btstack_core_event_mapper.c
|-- include/
|   |-- api/                     application ABI
|   `-- port/                    port implementation ABI
|       |-- btstack_platform.h
|       |-- btstack_worker_ops.h
|       `-- btstack_stack_ops.h
|-- transport/                   reusable H4/H5/SLIP and bus interfaces
`-- ports/
    `-- gpcv2178/
        |-- btstack_platform_gpcv2178.c
        |-- btstack_rtos_gpcv2178.c
        |-- btstack_run_loop_gpcv2178.c
        |-- btstack_uart_gpcv2178.c
        |-- btstack_control_gpcv2178.c
        |-- btstack_storage_gpcv2178.c
        `-- btstack_config.h
```

The existing `include/adapter` application-neutral headers remain compatible
during this refactor. Port-only contracts move to `include/port` as they are
introduced; unrelated public-header renaming is not part of the work.

`btstack/Makefile.inc` lists common sources and common include paths only. A
platform project appends its selected port sources and include directory. It
must not hard-code `ports/gpcv2178` in the common manifest.

GPCV2178 DAC, playback, and capture code moves to the demo/application audio
module. It does not move into the new common port ABI.

## Initialization and Deinitialization

Initialization follows this order:

1. Obtain `btstack_platform_default()`.
2. Validate platform ABI, structure size, and mandatory ops.
3. Initialize common static state and request/event/audio rings.
4. Call platform early initialization.
5. Bind system, storage, controller, and stack ops.
6. Initialize generic BTstack memory and the selected run loop.
7. Bind HCI transport, transport config, and controller control.
8. Initialize HCI, L2CAP, SDP, profiles, and the API event bridge.
9. Start the API and event workers with the existing priority and stack sizes
   supplied by the port.
10. Return from `bt_xstack_init()` without powering the controller.

`bt_xstack_enable()` retains the current sequence: start the BTstack run-loop
worker if it is not already running, then admit the existing asynchronous
power-on request. Repeated enable calls keep their current idempotent behavior.

Every completed stage records an internal lifecycle bit. Failure unwinds only
completed stages in reverse order and does not emit a READY event. The cleanup
entry point is internal and is used for initialization rollback and host-test
teardown. This design does not add a public deinitialization API;
`bt_xstack_disable()` retains its current controller-power behavior.

## Error Handling

Initialization fails with the existing `BT_XSTACK_INTERNAL_ERROR` for a null
platform descriptor, ABI/size mismatch, missing mandatory platform callback,
unpaired critical-section callbacks, or invalid worker/stack bindings. An
internal platform diagnostic records the exact validation reason. Invalid
fields supplied by the application in `bt_xstack_config_t` continue to return
`BT_XSTACK_INVALID_ARGUMENT`.

Runtime rules:

- A worker-start failure rolls back workers already started.
- HCI/controller initialization failure leaves the controller off and never
  reports READY.
- A failed worker notification leaves the item in its common ring, increments
  diagnostics, and permits a later wakeup to process it.
- Missing optional operations return internal `BTSTACK_STATUS_NOT_SUPPORTED`;
  each existing public API keeps its established mapping to the published
  `bt_xstack_result_t` values.
- Platform callbacks must not call application callbacks directly.
- Platform callbacks must not mutate common queue indices outside the common
  queue functions.

## Platform-Boundary Contract

Production files under these directories must not include or invoke RTOS,
chip, board, or vendor SDK APIs:

```text
btstack/api
btstack/core
btstack/gap
btstack/profiles
btstack/transport
```

The focused contract rejects, at minimum:

```text
FreeRTOS.h
task.h
queue.h
semphr.h
cmsis_os*
btstack_*gpcv2178*
gplib_*
drv_l1*
drv_l2*
taskENTER_CRITICAL
taskEXIT_CRITICAL
xQueue*
xTask*
```

Such dependencies are allowed in `btstack/ports/<platform>` and application
projects. Test stubs may intentionally contain these names. `btstack/src` is
the maintained upstream protocol stack and is not subjected to unrelated
rewrites by this refactor.

## Verification Strategy

### Host Mock Port

A mock RTOS port compiles and runs the common core without FreeRTOS or a vendor
SDK. It verifies descriptor validation, lifecycle ordering, worker wakeups,
rollback, and deinitialization.

### Queue Regression

Behavior tests preserve queue capacities, FIFO order, the four-slot control
reserve, PBAP backpressure/resume, reliable HFP-lost cleanup, and drop-counter
semantics.

### Execution Context

Instrumented workers verify that:

- Public request admission does not execute application callbacks inline.
- Ordinary events execute on the event worker.
- A2DP and HFP data callbacks execute on the stack worker.
- Worker notify happens outside the common critical section.

### Transport

H4, H5, and SLIP compile and run against mock UART interfaces without platform
headers. Static boundary checks reject vendor includes and concrete RTOS UART
instance declarations.

### Audio

Tests cover callback data lifetime, absent callbacks, fixed HFP TX capacity,
all-or-nothing admission, SCO can-send draining, disconnect cleanup, and no
dynamic allocation.

### GPCV2178

The existing PowerShell contracts, API and AT static-library builds, public
package checks, and main firmware link must remain green. Archive symbol and
source-path checks confirm that common code no longer imports FreeRTOS or
GPCV2178 while the selected port still supplies the required bindings.

## Migration Sequence

1. Add platform-boundary and mock-port contracts and verify the expected RED.
2. Define `btstack_platform_t` and have GPCV2178 populate it while retaining
   temporary compatibility bridges.
3. Route critical sections, time, delay, and logging through system ops.
4. Route API/event worker scheduling through worker ops without changing the
   common rings.
5. Route run-loop callbacks, HCI transport/control, and runtime lifecycle
   through stack/controller ops.
6. Remove vendor and concrete RTOS declarations from `transport`.
7. Move GPCV2178 application audio handling out of the port and implement the
   fixed HFP TX ring and additive write API.
8. Remove temporary GPCV2178/FreeRTOS compatibility paths from common code.
9. Remove the GPCV2178 include/source selection from the common manifest and
   make each platform project select exactly one port.
10. Regenerate API, AT, and firmware build files and run the complete focused
    and target-build verification matrix.

Every migration step must leave the GPCV2178 API library buildable and its
focused behavior tests green. The final boundary contract is not waived to
accommodate temporary dependencies.

## Success Criteria

The work is complete when:

- Common API/core/GAP/profile/transport production sources contain no RTOS,
  GPCV2178, or vendor SDK dependency.
- GPCV2178 is selected solely through its port package and exports the single
  platform descriptor symbol.
- A host mock port compiles and exercises the common lifecycle without
  FreeRTOS.
- Existing public ABI, queue capacities, event order, PBAP behavior, worker
  contexts, and GPCV2178 runtime behavior remain compatible.
- Audio downlink is delivered only through application callbacks, and HFP
  uplink uses the non-blocking 2048-byte common TX ring with a 255-byte maximum
  per public write.
- API, AT, package, and main firmware builds complete with no new warnings or
  errors attributable to this refactor.
