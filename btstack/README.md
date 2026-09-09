# BTstack Common Layer

This tree separates application-neutral Bluetooth behavior from the selected
board/RTOS implementation. `api`, `core`, `gap`, `profiles`, and `transport`
must not include FreeRTOS, vendor SDK, or GPCV2178 headers. `ports/gpcv2178`
is the first concrete implementation of the versioned `btstack_platform_t`
descriptor.

## Build selection

`Makefile.inc` contains common include paths and sources only. A target selects
exactly one port separately; GPCV2178 builds use
`ports/gpcv2178/Makefile.inc`. A final library or firmware must contain one
strong `btstack_platform_default()` definition. Do not combine two selected
port source lists or add a weak common fallback.

Consumers compose one common set, exactly one frontend, and one selected port:

```make
include $(BTSTACK_ROOT)/Makefile.inc
include $(BTSTACK_ROOT)/ports/gpcv2178/Makefile.inc

AT_LIBRARY_SOURCES := $(BTSTACK_COMMON_SOURCES) \
    $(BTSTACK_AT_FRONTEND_SOURCES) \
    $(BTSTACK_GPCV2178_PORT_SOURCES)
LEGACY_AT_LIBRARY_SOURCES := $(BTSTACK_COMMON_SOURCES) \
    $(BTSTACK_AT_CORE_SOURCES) \
    $(BTSTACK_GPCV2178_PORT_SOURCES)
API_LIBRARY_SOURCES := $(BTSTACK_COMMON_SOURCES) \
    $(BTSTACK_API_FRONTEND_SOURCES) \
    $(BTSTACK_GPCV2178_PORT_SOURCES)

AT_LIBRARY_CPPFLAGS := $(BTSTACK_AT_BUILD_CPPFLAGS)
LEGACY_AT_LIBRARY_CPPFLAGS := $(BTSTACK_AT_BUILD_CPPFLAGS)
API_LIBRARY_CPPFLAGS := $(BTSTACK_API_BUILD_CPPFLAGS)
```

`BTSTACK_AT_FRONTEND_SOURCES` adds the public `bt_xstack` AT entry to the six
internal `BTSTACK_AT_CORE_SOURCES`. Legacy/internal AT libraries select the
core set. `BTSTACK_AT_SOURCES` remains a deprecated alias for that core set.
`BTSTACK_SOURCES` is retained as a deprecated alias for
`BTSTACK_COMMON_SOURCES`; it intentionally does not select frontend code.
The GPCV2178 port requires exactly one of the two build-variant defines. This
compile-time selection keeps frontend-specific static worker stacks out of the
other archive. Static task-control blocks are selected the same way; they are
not embedded once per worker ID. This is not a runtime frontend registry.
Apply the selected `*_LIBRARY_CPPFLAGS` to every compilation unit in that
library target, including the selected port. Applying it only to frontend
sources leaves the port without a valid resource selection.

The five headers in `include/adapter` are application-neutral compatibility
interfaces and remain part of the common layer. The former physical
`btstack/adapter` implementation directory is obsolete; its sources now live
in `core`.

## Initialization and workers

`bt_xstack_init()` activates the selected descriptor, initializes the common
stack, and starts the API and event workers. `bt_xstack_enable()` then starts
the BTstack run-loop worker before submitting the asynchronous power-on
request. API calls only admit bounded requests; completion and ordinary
callbacks are delivered asynchronously by the event worker.

In the mutually exclusive AT build, frontend start creates the AT worker.
Before Bluetooth is enabled this is the only worker. A successful
`AT+BTEN=1` starts the STACK worker, so the steady worker set is AT + STACK;
the API/EVENT workers are neither allocated nor started. The AT frontend owns
three fixed common rings: Input 8 entries, Completion 1 entry, and Event 16
entries. Input and Event producers notify `BTSTACK_WORKER_SIGNAL_WORK`, while
backend completion notifies `BTSTACK_WORKER_SIGNAL_COMPLETION`. The AT worker
blocks indefinitely when idle instead of polling every 20 ms.

Command completion keeps the existing 1000 ms total timeout. Each completion
wake recomputes the remaining time, so unrelated/stale notifications do not
restart the deadline. Event ingress never blocks the BTstack run loop. If the
Event ring is full, that event is dropped, the overflow counter increments,
and queue-full is returned; accepted events are formatted and emitted by the
AT worker after the current command result.

The selected port supplies worker creation/wake/wait operations, critical
sections, time, run-loop/HCI/controller bindings, and optional storage and
identity services. Common code owns the fixed request, event, and audio rings.

## Audio ownership

A2DP media and HFP downlink audio are delivered directly on the BTstack worker
through application callbacks. Callback payload pointers are valid only for
the duration of the callback, so the application copies or consumes them
immediately. Ports must not perform product DAC/I2S playback or microphone
capture.

For HFP uplink, the application calls
`bt_xstack_call_audio_write(const uint8_t *data, uint32_t length)`. Admission
is non-blocking and all-or-nothing into a fixed 2048-byte common ring; a write
larger than 255 bytes is invalid. `BT_XSTACK_QUEUE_FULL` means the application
must retry later. SCO disconnect clears buffered uplink bytes.

## Basic BLE baseline

When `BTSTACK_LE` is enabled, the common layer provides one Legacy 1M LE link,
31-byte advertising and scan-response data, active/passive scanning, central
connect and peripheral advertising, Just Works security, GATT Client, GATT
Server, and optional HOGP Device support. GATT Client operations are serialized;
server registration and attribute values use fixed-capacity tables and pools.

Bond records use `le_device_db_memory` with `BTSTACK_LE_BOND_MAX == 4`. The TLV
singleton is linked only to satisfy the upstream ATT/SM interface; no persistent
TLV backend is installed, so bonds and persistent CCC values are RAM-only and
are cleared by a reboot. API and AT frontends share the same Adapter, Profile
Manager, GAP, and GATT implementations and remain mutually exclusive builds.

## Adding another port

Follow [ports/README.md](ports/README.md). In particular, implement one
descriptor, keep RTOS headers inside the port, provide static worker memory,
and verify both mock-host contracts and the target link.

## Verification

Run the PowerShell contracts under `api/tests`, `profiles/tests`, `core/tests`,
and `transport/tests`, then rebuild the selected library and target firmware.
