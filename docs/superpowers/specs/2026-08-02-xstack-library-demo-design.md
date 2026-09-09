# xStack Library Rename and WiFi-Gated Demo Design

## Goal

Rename the GPCV2178 Bluetooth host static library from `libbtstack.a` to
`libxstack.a`, and add a board application demo that starts xStack only after
WiFi startup succeeds.

## Architecture

The reusable xStack layer remains independent of WiFi and the ATBM vendor.
`bt_xstack_init()` and `bt_xstack_send_at()` remain the only application-facing
Bluetooth APIs. A new application-owned `bt_xstack_demo` module is responsible
for choosing the startup time and printing serialized AT replies and events.

The existing main application WiFi startup path calls
`bt_xstack_demo_start()` after the configured WiFi implementation reports a
successful startup. The current ATBM P2P path therefore becomes the trigger,
without introducing ATBM names or dependencies into xStack itself.

## Library Rename

- Change the BTstack library project output basename from `btstack` to
  `xstack`, producing `libxstack.a`.
- Change the main GPCV2178 project link input from `libbtstack.a` to
  `libxstack.a`.
- Update build integration tests and maintained documentation that describe
  the produced archive.
- Do not rename the `btstack/` source directory or third-party BTstack symbols;
  the rename applies only to the delivered library artifact.

## Demo Module

Create a generic application module with this entry point:

```c
int bt_xstack_demo_start(void);
```

On its first successful call, the function:

1. Calls `bt_xstack_init()` with a demo event callback.
2. Sends the exact command `AT+BTEN=1` through `bt_xstack_send_at()`.
3. Prints the initialization result, command submission result, and every
   callback payload using its explicit byte length.

The module uses a small internal state guard so repeated WiFi startup events do
not submit another enable command after the demo has started successfully. A
failed initialization or failed command submission is logged and remains
retryable on a later WiFi startup attempt.

The demo does not create another task. The xStack AT task handles the command,
and the first enable command creates the BTstack run-loop task according to the
existing manager lifecycle.

## WiFi Integration

The main application's `wifi_ap_mode_enable()` remains the owner of WiFi
startup sequencing. For the active ATBM configuration, it invokes
`bt_xstack_demo_start()` only when `atbm_wifi_p2p_mode_enable()` returns
`STATUS_OK`. A Bluetooth demo failure is logged but does not turn a successful
WiFi startup into a WiFi failure.

No call is added inside ATBM driver code, xStack manager code, or BTstack port
code. This preserves the reusable layer boundary.

## Error Handling

- `bt_xstack_demo_start()` returns the xStack status from initialization or
  command submission.
- The started guard is set only after `AT+BTEN=1` is accepted by the AT input
  queue.
- Callback output is treated as a byte span rather than a NUL-terminated
  string.
- Existing xStack behavior remains unchanged: HCI power-on and run-loop start
  occur only in response to `AT+BTEN=1`.

## Build and Test Strategy

- Add a failing contract test for the `libxstack.a` output/link names and the
  absence of active `libbtstack.a` project references.
- Add a failing demo contract test for the independent module, callback,
  initialization, enable command, retryable guard, and WiFi-success call site.
- Build the static library and verify that `libxstack.a` exports
  `bt_xstack_init` and `bt_xstack_send_at`.
- Build the main Debug project and verify that the final ELF contains
  `bt_xstack_demo_start`, `bt_xstack_init`, and `bt_xstack_send_at`.
- Run all existing Bluetooth contract tests as regression coverage.

Board-level UART/H5 communication still requires hardware verification after
the build is flashed.
