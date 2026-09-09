# BTstack port checklist

Each target build selects exactly one port in addition to the common sources.
A new port must satisfy all of the following:

1. Define exactly one strong `btstack_platform_default()`; do not add a weak
   fallback or runtime platform registry.
2. Use static or application-supplied worker memory, and keep all RTOS headers
   and primitives out of common code.
3. Bind the mandatory system critical/time operations, workers, run loop, HCI
   transport, and controller through `btstack_platform_t`.
4. Treat storage and address/identity services as optional and implement their
   null behavior explicitly.
5. Keep application audio playback and capture out of the port. Media and call
   audio remain application-owned callbacks; common code owns HFP TX buffering.
6. Pass the platform-boundary and mock-port tests, rebuild the selected static
   library, and verify the final target link and descriptor symbol count.

`gpcv2178/Makefile.inc` is the reference selected-port manifest. Combine it
with `BTSTACK_COMMON_SOURCES` and exactly one of
`BTSTACK_AT_FRONTEND_SOURCES` or `BTSTACK_API_FRONTEND_SOURCES`. A legacy
internal AT library uses `BTSTACK_AT_CORE_SOURCES` instead of the public AT
frontend. Common
`btstack/Makefile.inc` intentionally contains no path below `ports/gpcv2178`.

The GPCV2178 worker port also requires one explicit, mutually exclusive
compile-time selection:

- API libraries apply `BTSTACK_API_BUILD_CPPFLAGS`, which defines
  `BTSTACK_BUILD_VARIANT_API=1`, to the whole target. The port allocates the
  fixed API and EVENT stacks and their API and EVENT task controls, but no AT
  stack or task control.
- Public and legacy AT libraries apply `BTSTACK_AT_BUILD_CPPFLAGS`, which
  defines `BTSTACK_BUILD_VARIANT_AT=1`, to the whole target. The port
  allocates one fixed 2048-word (8192-byte) AT stack and one AT task control,
  but no API/EVENT stacks or task controls.

Defining both or neither is a build error. The shared worker-ops ABI still
contains API, EVENT, STACK, and AT worker IDs; a selected port rejects worker
IDs whose resources are not present. In a pure AT build the AT worker starts
with the frontend and the dynamically created STACK run-loop worker starts
only after `AT+BTEN=1`.
