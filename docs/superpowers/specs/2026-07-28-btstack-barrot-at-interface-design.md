# BTstack BARROT AT Interface Design

Date: 2026-07-28

Source protocol:
`E:/Project/ark/at_doc/BARROT_AT_command&event_260310_V1.2.pdf`

## 1. Context

The GPCV2178 port already provides:

- An ATBM controller transport over H4 or H5.
- A FreeRTOS BTstack run loop task.
- Controller power and reset integration.
- Classic GAP initialization.
- A2DP Sink, HFP Hands-Free, AVRCP Controller, and AVRCP Target
  registration.
- Basic profile event handlers and diagnostic logging.

The current port does not provide a HOST-facing BARROT AT command interface.
A2DP media packets are intentionally discarded, HFP SCO audio is not routed,
and several profiles in the BARROT command set are not initialized.

## 2. Goals

The first phase will:

- Expose a transport-independent API that accepts one complete AT command
  string from an upper-layer FreeRTOS task.
- Copy accepted commands into a static queue.
- Parse commands in a dedicated `btstack_at` task.
- Execute every BTstack operation in the existing BTstack run-loop task.
- Return command responses and asynchronous events through one text callback
  that always runs in the `btstack_at` task.
- Match the confirmed BARROT syntax, response ordering, separators, and line
  endings.
- Support the confirmed Classic, HFP HF, A2DP Sink, and basic AVRCP command
  subset.
- Keep all AT configuration and paired-device metadata in RAM.

## 3. Non-goals

The first phase will not:

- Bind the AT interface to UART, USB CDC, a socket, or another transport.
- Accept a byte stream or assemble commands from fragments.
- Support invocation from an interrupt.
- Decode or play A2DP media.
- Route HFP SCO data to a microphone or speaker.
- Support BLE commands, GATT, SPP, PBAP, HID, ANCS, AAP, or iAP2.
- Support AVRCP browsing or BIP cover-art transfer.
- Persist settings or paired-device metadata across reboot.
- Rebuild the BTstack task for `AT+REBOOT` or `AT+RESTORE`.

## 4. Public API

The public header will expose:

```c
typedef void (*btstack_at_output_callback_t)(
    const char *data,
    uint16_t length,
    void *context);

typedef struct {
    btstack_at_output_callback_t output_callback;
    void *output_context;
    UBaseType_t task_priority;
    uint16_t task_stack_size;
} btstack_at_config_t;

int btstack_at_init(const btstack_at_config_t *config);
int btstack_at_start(void);

int btstack_at_submit(
    const char *command,
    uint16_t length);
```

API rules:

- `btstack_at_init()` validates and copies the configuration. It is
  idempotent only when called again with the same configuration before start;
  incompatible repeated initialization fails.
- `btstack_at_start()` creates the static queues and dedicated AT task. A
  repeated call after a successful start returns success without creating a
  second task.
- `btstack_at_submit()` may be called from an ordinary FreeRTOS task. It must
  not be called from an ISR.
- `btstack_at_submit()` copies the input before returning. The caller may
  immediately reuse or release its source buffer.
- The submit return value reports only acceptance into the input queue:
  success, not initialized, invalid argument, command too long, or queue full.
- An accepted command always produces a final protocol response through the
  output callback.
- Callback data is valid only for the duration of the callback.
- The callback must not block for a long period or call `btstack_at_submit()`
  synchronously in a way that can deadlock its own task.

## 5. Task and Concurrency Architecture

```text
Upper-layer task
    |
    | btstack_at_submit(): validate envelope, copy, enqueue
    v
Dedicated btstack_at task
    |
    | dequeue, parse, validate, create normalized operation
    v
btstack_run_loop_execute_on_main_thread()
    |
    | execute GAP/HFP/A2DP/AVRCP operation
    v
Existing btstack_gpcv2178_task
    |
    | completion and normalized asynchronous events
    v
Dedicated btstack_at task
    |
    | format BARROT text
    v
Upper-layer output callback
```

The dedicated AT task must not directly call APIs that mutate or inspect
BTstack protocol state. It submits one normalized operation through
`btstack_run_loop_execute_on_main_thread()`. The GPCV2178 run loop already
wakes its task and executes registered main-thread callbacks.

Only one AT operation may be in flight. The callback registration and
operation storage can therefore be statically allocated and reused after
completion.

The BTstack callback posts an immediate operation result to a one-entry
completion queue. Connection, disconnection, discovery, call, and playback
completion remain asynchronous and are reported later as protocol events.

While an operation is in flight, normalized asynchronous events accumulate in
the event queue. The AT task emits the command response first, then drains
those events. This prevents an immediately generated event from appearing
before the accepting `OK`.

The unified output callback is invoked only by the AT task. No HCI, profile,
timer, or BTstack run-loop callback may call it directly.

## 6. Static Resources and Limits

Defaults are overridable at build time:

```c
#define BTSTACK_AT_COMMAND_MAX       128u
#define BTSTACK_AT_INPUT_QUEUE_LEN     8u
#define BTSTACK_AT_EVENT_QUEUE_LEN    16u
#define BTSTACK_AT_OUTPUT_MAX         512u
#define BTSTACK_AT_EXEC_TIMEOUT_MS   1000u
```

The input queue, completion queue, event queue, task control block, task stack,
command storage, operation storage, and formatting buffer use static memory.
No AT command path requires heap allocation.

The execution timeout covers only dispatch to and return from the BTstack
run-loop callback. It does not wait for a remote device. A generation number
identifies the in-flight operation so a late completion cannot be associated
with a later command.

The BTstack run loop must never block waiting for the AT task. If the event
queue is full, the new event is dropped and an overflow diagnostic counter is
incremented. Current state remains recoverable through status query commands.

## 7. Supported Command Set

### 7.1 General and Classic

- `AT`
- `AT+PROFILE`
- `AT+VER`
- `AT+BTEN`
- `AT+ADDR`
- `AT+NAME`
- `AT+PIN`
- `AT+SSP`
- `AT+CFM`
- `AT+PAIR`
- `AT+PAGE`
- `AT+COD`
- `AT+PLIST`
- `AT+TDL`
- `AT+SCAN=0`
- `AT+SCAN=1{,period{,name}}`
- `AT+DSCA`
- `AT+MICMUTE`
- `AT+SPKVOL`

`AT+PROFILE` reports and accepts only decimal mask `424`, which represents
HFP Sink, A2DP Sink, AVRCP Controller, and AVRCP Target. Any other mask returns
`ERROR`.

`AT+VER` reports the compile-time string `BTSTACK_AT_FIRMWARE_VERSION`. The
GPCV2178 first-phase default is `GPCV2178_BTSTACK_1.0`.

`AT+NAME=<name>,1` appends a hyphen and the final four hexadecimal characters
of the local BR/EDR address. The combined advertised name must still fit the
BTstack local-name limit.

`AT+PAIR` without a parameter is equivalent to `AT+PAIR=1`. Only Classic modes
0 and 1 are supported; modes 2 and 3 return `ERROR`.

`AT+PAGE` without a parameter is equivalent to `AT+PAGE=1`.

`AT+CFM` succeeds only while SSP mode 3 has a pending confirmation request for
the supplied MAC address.

The RAM trust-device list contains at most eight entries. `AT+TDL=0` uses
automatic replacement of the oldest entry. Values 1 through 8 prevent
discoverability when the configured limit is full.

Classic scan supports only modes 0 and 1. Scan periods and name filters follow
the PDF ranges. Modes 2 and 3 return `ERROR`.

`AT+PLIST` emits zero or more records, then `+PLIST=E`, then `OK`. Clear by
index and clear by MAC use the exact forms defined by the PDF.

`AT+MICMUTE` is valid only during an active HFP call. It changes the reported
control state but does not route or alter PCM samples in the first phase.

### 7.2 HFP Hands-Free

- `AT+HFPCFG`
- `AT+HFPSTAT`
- `AT+HFPCONN`
- `AT+HFPDISC`
- `AT+HFPDIAL`
- `AT+HFPDTMF`
- `AT+HFPANSW`
- `AT+HFPCHUP`
- `AT+HFPMCAL`
- `AT+HFPADTS`
- `AT+HFPVR`

No-address connect commands use the most recently connected RAM peer. If no
peer exists, they return `ERROR`.

`AT+HFPADTS` controls SCO connection state and produces the corresponding
event, but SCO samples are not routed to audio hardware.

### 7.3 A2DP Sink and AVRCP

- `AT+A2DPSTAT`
- `AT+A2DPCONN`
- `AT+A2DPDISC`
- `AT+A2DPMUTE`
- `AT+AVRCPCFG`
- `AT+PLAYPAUSE`
- `AT+PLAY`
- `AT+PAUSE`
- `AT+STOP`
- `AT+NEXT`
- `AT+PREV`
- `AT+FORWARD`
- `AT+BACKWARD`
- `AT+REPEAT`
- `AT+SHUFFLE`
- `AT+SABSUP`
- `AT+SABSDOWN`

`AT+AVRCPCFG` supports automatic metadata and play-progress settings in bits
0 through 3. Browsing and cover-art bits 4 and 5 are rejected.

`AT+PLAYPAUSE` sends Play when the cached remote state is stopped or paused,
and Pause when it is playing, fast-forwarding, or rewinding. An unknown remote
play state returns `ERROR`.

`AT+REPEAT=0/1` maps to repeat Off/All Tracks.
`AT+SHUFFLE=0/1` maps to shuffle Off/All Tracks.
Fast-forward and rewind controls send a complete press/release pass-through
operation. Volume-up and volume-down commands send AVRCP pass-through volume
operations and rely on the remote `+ABSVOL` notification for the final value.

A2DP may report Streaming state while received media packets are intentionally
discarded.

### 7.4 Explicitly Unsupported

The parser recognizes commands outside the whitelist so it can return the same
plain `ERROR` as malformed or unavailable commands. Unsupported groups are:

- `AT+LEADDR`, `AT+LENAME`, and BLE scan modes.
- `AT+REBOOT` and `AT+RESTORE`.
- AVRCP browsing and BIP commands: `GETMP`, `SETMP`, `GETFD`, `SETFD`,
  `GETNP`, `ADDMP`, and `GBIPART`.
- All PBAP, SPP, GATT, HID, ANCS, AAP, and iAP2 commands.

## 8. Parsing and Protocol Compatibility

Input contains exactly one command. A final `\r\n` is optional because the API
already provides command boundaries. Output always uses `\r\n`.

The parser rejects:

- Embedded CR or LF characters.
- Embedded NUL characters within the supplied length.
- Multiple concatenated commands.
- Lowercase or mixed-case command names.
- Internal or leading whitespace.
- Unsupported query, set, or execute forms.
- Extra parameters, empty required parameters, out-of-range values, and
  trailing separators.

Numeric parameters are decimal unless the PDF explicitly defines a hexadecimal
field. MAC addresses contain exactly 12 hexadecimal characters. Input MAC
letters may use either case; output is uppercase.

The PDF token `<FF>` is encoded as ASCII Form Feed byte `0x0C`. It is not the
five-character string `"<FF>"` and is not byte `0xFF`.

Confirmed document corrections are:

- Accept `AT+HFPMCAL`, not the example's spaced `AT+ HFPMCAL`.
- Accept `AT+AVRCPCFG`, not the example's spaced form.
- Use HFP state values 0 through 10 because those values are enumerated even
  though one range annotation ends at 6.
- Emit `+HFPVR`, correcting the HFP voice-assistant format typo.
- Emit `+TRACKINFO`, which is used by the formal format and example even
  though the section title says `+MEDIAINFO`.
- Do not accept other example-only spacing mistakes.

## 9. Command Response Semantics

Responses use these forms:

```text
Query success:    +XXXX=...\r\nOK\r\n
Set success:      OK\r\n
Execute success:  OK\r\n
Any failure:      ERROR\r\n
```

`AT+A2DPMUTE` returns the documented state response followed by success:

```text
+A2DPMUTED=<0|1>\r\nOK\r\n
```

`OK` for an asynchronous command means that the local BTstack API accepted the
operation. It does not mean that a remote device connected, disconnected,
answered, or changed playback state. Final state is reported through
asynchronous events.

A command returns `ERROR` for:

- Syntax or parameter failure.
- An unsupported command or option.
- A command that is invalid in the current state.
- A missing remembered peer for an address-less reconnect.
- An immediate BTstack API failure.
- A run-loop dispatch timeout.

The implementation does not introduce vendor error numbers because the source
protocol defines only `ERROR`.

## 10. RAM Configuration and Defaults

Configuration state is owned by the AT backend and reset when it is
initialized. Compatible values use the BARROT defaults:

- BR/EDR name: `bluetooth_auto`
- Profile mask: `424`, constrained to the implemented profiles
- Class of device: `240404`
- Trust-device-list mode: `0`
- HFP configuration: `3`
- AVRCP configuration: `3`
- Speaker volume: `14`
- Microphone mute: `0`
- A2DP mute: `0`
- PIN: `0000`
- SSP mode: `2`

PIN accepts 4 through 15 ASCII bytes. SSP and PIN defaults are applied to GAP
when the controller becomes working. Changes are never written to the existing
storage backend.

Pairing success updates the RAM peer list and most-recent-peer selection.
Restarting the AT component clears that list and restores defaults.

## 11. State and Event Mapping

The backend owns a state snapshot that is mutated only in the BTstack run-loop
task. Query commands obtain a copied snapshot as their immediate operation
result.

Event mappings are:

- BTstack/HCI power and GAP mode changes:
  `+PWRSTAT`, `+DEVSTAT`
- Classic inquiry:
  `+SCAN`, followed by `+SCAN=E`
- SSP and authentication:
  `+PAIRREQ`, `+PAIRED`
- Microphone control:
  `+MICMUTED`
- HFP service, calls, audio, and AG indicators:
  `+HFPSTAT`, `+HFPDEV`, `+HFPAUDIO`, `+HFPVR`, `+HFPSIG`,
  `+HFPNET`, `+HFPROAM`, `+HFPBATT`, `+HFPNUM`, `+HFPIBR`,
  `+HFPRING`, `+HFPLOST`, `+HFPMTPX`
- A2DP connection and streaming:
  `+A2DPSTAT`, `+A2DPDEV`, `+A2DPLOST`
- AVRCP connection, playback, metadata, and volume:
  `+AVRCPSTAT`, `+PLAYSTAT`, `+PLAYMODE`, `+TRACKSTAT`,
  `+TRACKINFO`, `+ABSVOL`

State events are emitted only when their value changes. Scan results, pairing
requests, pairing results, ringing, call multiplex information, and metadata
are occurrence events and are not deduplicated.

An explicitly requested disconnect does not produce `+HFPLOST` or
`+A2DPLOST`. Those events are reserved for unexpected link loss.

AVRCP metadata requests and configured progress polling use BTstack run-loop
timers. The AT task never polls profile state directly.

## 12. Component Layout

New files:

- `btstack/include/at/btstack_at.h`
  - Public configuration, callback, status, initialization, start, and submit
    API.
- `btstack/at/btstack_at.c`
  - Static resources, task entry, input processing, operation lifecycle,
    response priority, event draining, and callback invocation.
- `btstack/at/btstack_at_parser.c`
  - Dependency-free command recognition and parameter validation.
- `btstack/at/btstack_at_protocol.c`
  - BARROT response and event formatting.
- `btstack/at/btstack_at_internal.h`
  - Normalized commands, operation results, events, state snapshots, and
    internal contracts.
- `btstack/ports/gpcv2178/btstack_at_gpcv2178.c`
  - Run-loop dispatch, BTstack API mapping, RAM state, timers, and native event
    conversion.
- `btstack/ports/gpcv2178/btstack_at_gpcv2178.h`
  - GPCV2178 backend registration and dispatch declarations.

Existing files changed during implementation:

- `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
  - Register the backend with the initialized run loop.
- `btstack/ports/gpcv2178/btstack_gpcv2178_profiles.c`
  - Forward relevant existing and newly handled profile events into the AT
    backend without formatting text there.
- `btstack/adapter/Makefile.inc` or the target project source list
  - Compile the AT core and GPCV2178 backend.
- The GPCV2178 project file
  - Add new source files and include path if the project does not consume the
    make source list.

The core parser and formatter must not include GPCV2178, HCI transport, board,
or product headers.

## 13. Verification Strategy

### 13.1 Parser Tests

Table-driven host tests cover:

- Every supported command in query, set, and execute forms.
- Minimum, maximum, and out-of-range numeric parameters.
- MAC address case and length.
- Optional command-ending CRLF.
- Embedded control characters and concatenated commands.
- Unsupported profiles and commands.
- Every confirmed PDF typo correction.

### 13.2 Protocol Formatting Tests

Golden byte arrays verify:

- Exact `\r\n` placement.
- Query value followed by `OK`.
- Plain `ERROR`.
- Uppercase MAC output.
- `0x0C` field separators.
- `+HFPVR` and `+TRACKINFO` naming.
- Multi-record `+PLIST` output and final `OK`.

### 13.3 Concurrency and Backend Tests

A fake run-loop backend verifies:

- Input is copied before `submit()` returns.
- Queue-full behavior does not corrupt accepted commands.
- BTstack operations execute only through the main-thread callback.
- Only one operation is in flight.
- Command completion is output before events caused by that command.
- Late completions are rejected by generation number.
- Dispatch timeout produces one `ERROR`.
- Event overflow does not block the BTstack callback.
- The user callback runs only in the AT task.

### 13.4 SDK and Target Verification

The implementation must pass the focused existing BTstack compile and boundary
checks and compile in the GPCV2178 project.

Board acceptance covers:

- Basic `AT`, version, address, name, profile, and state queries.
- Classic discovery start, results, filtering, and stop.
- Pair request accept/reject and RAM paired-list management.
- HFP connect, incoming/outgoing call controls, indicators, and disconnect.
- A2DP connect, stream-state reporting, mute state, and disconnect.
- AVRCP playback, navigation, repeat/shuffle, metadata, progress, and volume.
- Rejection of every explicitly unsupported command group.
- Burst submission from another FreeRTOS task without response reordering.

Audio playback and SCO audio quality are explicitly excluded from acceptance.

## 14. Acceptance Criteria

The first phase is complete when:

- The public API can be called from an upper-layer FreeRTOS task without
  requiring transport-specific code.
- Accepted commands are copied and processed by the dedicated AT task.
- Every BTstack operation runs in the BTstack run-loop task.
- Every supported command has a deterministic response.
- Every unsupported or invalid command returns exactly `ERROR\r\n`.
- Responses and events match the confirmed protocol byte format.
- The unified output callback runs only in the AT task.
- Command responses do not reorder under burst submission.
- The approved Classic, HFP HF, A2DP Sink, and basic AVRCP board flows work.
- No implementation claims or depends on actual A2DP or SCO audio routing.
