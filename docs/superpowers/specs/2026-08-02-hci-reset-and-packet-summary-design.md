# GPCV2178 HCI Reset and Packet Summary Design

## Goal

During GPCV2178 Bluetooth initialization, submit exactly one logical HCI Reset command and wait for its Command Complete event before the HCI state machine submits the next command. Preserve H5 reliable-frame retransmission. Improve diagnostics so HCI Command and Event packets are printed in full, while data-bearing packet types are summarized without payload dumps.

## Evidence and Root Cause

`Serial Debug 2026-08-02 060050.txt` records three logical Reset submissions at lines 305-307 before the first Reset Command Complete at line 308:

```text
HCI TX CMD len=3: 03 0C 00
HCI TX CMD len=3: 03 0C 00
HCI TX CMD len=3: 03 0C 00
HCI RX EVT len=6: 0E 04 01 03 0C 00
```

The source is the generic HCI initialization resend timer. In `HCI_INIT_SEND_RESET`, `hci.c` arms a 200 ms timer. If Command Complete has not arrived, the timeout handler returns the state machine to `HCI_INIT_SEND_RESET`, restores one command credit, and submits Reset again.

This is separate from H5 reliable-frame retransmission. H5 must retain its queued reliable frame and resend it until acknowledged.

## Reset Behavior

Add a compile-time switch whose generic default keeps the existing BTstack behavior. The GPCV2178 configuration disables only the initial logical HCI Reset resend timer.

When disabled:

1. `HCI_INIT_SEND_RESET` submits `hci_reset` once.
2. The HCI state remains `HCI_INIT_W4_SEND_RESET`.
3. No HCI-level Reset resend timer is armed.
4. A matching Command Complete advances to Read Local Version Information.
5. H5 ACK handling and reliable-frame timeout retransmission remain unchanged.

The switch must not disable timers used by baud-rate changes, chipset warm boot flows, or H5 link reliability.

## Packet Logging

The GPCV2178 HCI dump backend owns normal packet logging.

### Complete packets

- Outgoing HCI Command: print direction, type, length, and every byte.
- Incoming HCI Event: print direction, type, length, and every byte.
- Construct the hexadecimal text first and submit it with one debug-print call so another task cannot interleave text between individual bytes.
- The platform debug print buffer must hold the maximum valid HCI Command/Event line without truncation; 1024 bytes is sufficient for the 258-byte command maximum rendered as spaced hexadecimal.

### Summary-only packets

- ACL, both directions: direction, packet length, connection handle, PB flag, BC flag, ACL data length. When the packet contains a first-fragment L2CAP header, also print L2CAP length and CID.
- SCO, both directions: direction, packet length, connection handle, packet-status flag, and SCO data length.
- ISO, both directions: direction, packet length, connection handle, PB flag, timestamp flag, and ISO data length.
- Never print ACL, SCO, or ISO payload bytes in normal mode.
- Malformed or short packets still produce a safe summary containing direction and available length; the logger must not read beyond the supplied buffer.

Existing H5-specific ACL payload-prefix diagnostics are restricted to `BTSTACK_H5_DIAG_VERBOSE`. The normal GPCV2178 setting remains zero, preventing duplicate ACL logs.

## Expected Log Shape

```text
HCI TX CMD len=3: 03 0C 00
HCI RX EVT len=6: 0E 04 01 03 0C 00
HCI RX ACL len=28 handle=0x0001 pb=2 bc=0 acl_len=24 l2cap_len=20 cid=0x0041
HCI TX SCO len=... handle=0x.... status=... data_len=...
HCI RX ISO len=... handle=0x.... pb=... ts=... data_len=...
```

Only one `HCI TX CMD len=3: 03 0C 00` is expected from the logical HCI layer during normal initialization. H5 can still retransmit its reliable wire frame without generating another logical HCI dump entry.

## Verification

Automated contract tests must first fail against the current implementation, then verify:

- GPCV2178 disables only logical initial Reset resend.
- Generic BTstack retains Reset resend by default.
- Command and Event full-dump paths remain present.
- ACL/SCO/ISO summary paths exist and do not iterate over their payloads.
- Legacy H5 ACL detail calls are verbose-only.

Build verification must recreate `libxstack.a`, link the Debug main project, regenerate `project.bin`, and confirm the new Reset-policy and packet-summary strings are present in the final ELF/BIN.

Hardware acceptance requires a fresh boot log showing one logical Reset line, its Command Complete before the next HCI command, and summary-only ACL/SCO/ISO output.
