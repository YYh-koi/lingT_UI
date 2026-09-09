# Bluetooth Demo Name Command Design

## Goal

When the Bluetooth application demo starts, submit the following command before enabling Bluetooth:

```text
AT+NAME=GP-BT-ATBM6165
```

Then submit the existing enable command:

```text
AT+BTEN=1
```

## Scope

- Change `Project/wifi_demo/bt_xstack_demo.c` only for production behavior.
- Update the demo contract test to verify the exact name command and its ordering.
- Do not change the BTstack property default, AT backend default, SDP service names, or Wi-Fi naming.
- The name is set when `bt_xstack_demo_start()` is used; other applications remain responsible for setting their own name.

## Startup Sequence

1. Initialize XStack.
2. Start the platform audio task.
3. Register the A2DP PCM callback.
4. Submit `AT+NAME=GP-BT-ATBM6165`.
5. Submit `AT+BTEN=1`.

The AT input queue processes commands in FIFO order. Its worker waits for the current command result before processing the next queued item, so the name command completes before the enable command is handled.

## Error Handling

- If submitting the name command fails, stop the audio task, return the error, and do not submit `AT+BTEN=1`.
- Preserve the existing error handling for Bluetooth enable failure.
- Log the name-command submission result for board-side diagnosis.

## Verification

- First extend `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1` so it fails without the new behavior.
- Verify the demo contains the exact name command.
- Verify the name command is submitted before `AT+BTEN=1`.
- Run the focused demo contract test after implementation.
- Rebuild the main application ELF/BIN; rebuilding `libxstack.a` is not required because this change is application-only.
- On hardware, verify the log shows the name command before Bluetooth enable and that inquiry reports `GP-BT-ATBM6165`.
