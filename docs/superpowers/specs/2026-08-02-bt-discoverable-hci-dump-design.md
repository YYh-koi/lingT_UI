# Bluetooth Discoverability and HCI Dump Design

## Goal

After `AT+BTEN=1` completes, the local Classic Bluetooth device shall be powered, connectable, and discoverable. UART1 shall print HCI Command and HCI Event packets as hexadecimal diagnostic lines.

## Current behavior and root cause

- The configured Class of Device is `0x240404`: Audio and Rendering service classes, Audio/Video major class, Wearable Headset minor class.
- Classic profile setup calls the native `gap_connectable_control(1)` and `gap_discoverable_control(1)` APIs.
- The adapter GAP cache still contains its zero-initialized `connectable=false` and `discoverable=false` values.
- When HCI reaches `HCI_STATE_WORKING`, `btstack_gap_link_apply_cached()` reapplies those false values. The resulting event is `+DEVSTAT=1`, which means powered but not discoverable.

## Discoverability change

The AT backend initialization will set the desired startup policy through the adapter APIs:

- `btstack_gap_set_connectable(true)`
- `btstack_gap_set_discoverable(true)`

Using the adapter APIs updates both the BTstack controller request and the cached GAP state. When HCI reaches WORKING and reapplies the cache, the device remains connectable and discoverable. The expected startup status is `+DEVSTAT=3`.

The existing `AT+PAIR=0/1` and `AT+PAGE=0/1` commands remain able to change discoverability and connectability after startup.

## HCI dump output

A GPCV2178 HCI dump backend will be added under `btstack/ports/gpcv2178` and registered before HCI initialization.

The backend will:

- print outgoing HCI Command packets;
- print incoming HCI Event packets;
- omit ACL, SCO, and ISO payload packets;
- include direction, packet kind, length, and hexadecimal bytes;
- use the existing UART1 `DBG_PRINT` path;
- be controlled by a compile-time macro that is enabled for the current debug build.

Example output:

```text
HCI TX CMD len=3: 03 0C 00
HCI RX EVT len=6: 0E 04 01 03 0C 00
```

Filtering ACL/SCO/ISO avoids flooding UART1 during A2DP or HFP streaming and reduces timing impact.

## Tests and verification

1. Add a contract test that initially fails unless startup policy uses the adapter connectable/discoverable APIs.
2. Add a contract test that initially fails unless the GPCV2178 dump backend is registered and filters output to Command/Event packets.
3. Rebuild `BTstack_Library` and verify `libxstack.a` is newer than all changed BTstack sources.
4. Rebuild the main project and verify the generated `project.bin` is newer than `libxstack.a`.
5. On hardware, verify the log reaches `+PWRSTAT=1`, then `+DEVSTAT=3`, and shows HCI TX CMD/HCI RX EVT lines.
6. Confirm a phone can discover `bluetooth_auto` and initiate a connection.

## Non-goals

- No binary btsnoop file output.
- No ACL/SCO/ISO payload dump.
- No change to CoD, local name, UART pinmux, H5 transport, or Bluetooth firmware image.
