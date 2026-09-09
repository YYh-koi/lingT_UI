# HCI Reset One-Second Retry Design

## Goal

Restore the BTstack logical HCI Reset retry and increase its timeout from
200 ms to 1000 ms for the GPCV2178 port.

## Behavior

- Submit the first HCI Reset when controller initialization begins.
- Wait up to 1000 ms for the matching HCI Reset Command Complete event.
- If it does not arrive, submit HCI Reset again and restart the timeout.
- After the matching event arrives, cancel the timer and advance to the next
  initialization command.
- Other regular initialization commands remain single-submit and advance only
  after their matching completion event.
- Preserve H5 sequence numbers, acknowledgements, duplicate-frame filtering,
  and reliable transport retransmission.

This intentionally means that multiple logical Reset commands can appear in
the HCI dump when the controller does not complete Reset within one second.

## Implementation

- Set `HCI_RESET_RESEND_ENABLE` to `1` in the GPCV2178 BTstack config.
- Set `HCI_RESET_RESEND_TIMEOUT_MS` to `1000` in the same port config.
- Keep the generic BTstack defaults and timer implementation configurable for
  other ports.

## Verification

- Replace the current single-Reset contract with a one-second retry contract.
- Verify the revised test fails before the production configuration change.
- Run all focused BTstack tests.
- Rebuild `libxstack.a` and the main ELF/BIN.
- Confirm the final library uses the 1000 ms timeout configuration and still
  includes H5 reliable transport.
