# Bluetooth AVRCP Media Key Design

## Goal

Use three existing board ADKEY buttons to control Bluetooth music without changing the existing call-answer and call-hangup key behavior.

## Key Mapping

Only short presses are intercepted for Bluetooth media control:

| Board key | Bluetooth action | AT command |
|---|---|---|
| MENU | Previous track | `AT+PREV` |
| MODE | Play/pause toggle | `AT+PLAYPAUSE` |
| NEXT | Next track | `AT+NEXT` |

Long-press behavior remains unchanged. `OK` continues to answer calls, `PREV` continues to hang up calls, and `SOS` is not changed.

## Architecture

The peripheral key module remains independent of Bluetooth. Add a generic media-key callback interface beside the existing call-key callback interface:

- `ap_peripheral_media_key_action_t` identifies previous, play/pause, and next actions.
- `ap_peripheral_media_key_handler_t` returns nonzero when the application consumes a key.
- `ap_peripheral_media_key_handler_register()` installs the application handler.

The short-press branches of `ap_peripheral_menu_key_exe()`, `ap_peripheral_mode_key_exe()`, and `ap_peripheral_next_key_exe()` dispatch the corresponding generic action before their existing behavior. When the handler returns zero, all original application behavior continues unchanged.

`bt_xstack_demo` registers the media handler and translates the generic actions to existing AVRCP AT commands through `bt_xstack_send_at()`.

## AVRCP State Gate

`bt_xstack_demo` parses the existing AT event `+AVRCPSTAT=<state>`:

- `+AVRCPSTAT=3`: AVRCP connected; media keys may be consumed.
- `+AVRCPSTAT=1` or `+AVRCPSTAT=2`: AVRCP not ready; media keys are not consumed.
- `+PWRSTAT=0`: clear the AVRCP-connected state.

When AVRCP is not connected, the media handler returns zero and the original MENU, MODE, or NEXT behavior runs.

When AVRCP is connected, the handler submits the mapped AT command and returns nonzero. A submission failure is logged but still consumes the key, preventing an unrelated original application action from running after a Bluetooth control attempt.

## Event and Command Flow

```text
AVRCP profile state
  -> +AVRCPSTAT event
  -> bt_xstack_demo event callback
  -> demo AVRCP-connected flag

ADKEY short press
  -> peripheral generic media action
  -> bt_xstack_demo media handler
  -> AVRCP state gate
  -> bt_xstack_send_at()
  -> AT backend
  -> btstack_avrcp_control()
```

## Concurrency

The AVRCP-connected flag is shared by the AT event callback and the peripheral key path. Reads and writes use the same FreeRTOS critical-section pattern already used by the demo start state. `bt_xstack_send_at()` is called outside a critical section.

## Compatibility

- No public `bt_xstack.h` API changes.
- No AT command, parser, backend, or AVRCP profile changes.
- No changes to PCM delivery.
- No changes to current call key actions.
- Original key behavior is preserved whenever AVRCP is not connected.

## Verification

Add contract tests that prove:

1. The generic media-key enum, callback type, and registration function exist.
2. The peripheral module does not include Bluetooth headers.
3. MENU, MODE, and NEXT short presses dispatch previous, play/pause, and next respectively before existing behavior.
4. `bt_xstack_demo` registers the media handler.
5. The demo maps actions to `AT+PREV`, `AT+PLAYPAUSE`, and `AT+NEXT`.
6. The demo consumes media keys only when `+AVRCPSTAT=3` has been observed and clears readiness on other AVRCP states or power-off.
7. The Bluetooth library and main project still build successfully.

Board flashing and physical key validation remain a separate hardware verification step.
