# Bluetooth Default Local Name Design

## Goal

Set the xStack default Bluetooth local name to `GP-BT-ATBM6165`.

## Scope

- Change the central xStack property-layer default name.
- Use the same default for Classic and BLE local-name properties.
- Preserve runtime name changes through the existing AT command path.
- Do not change the Wi-Fi SSID, Wi-Fi P2P device name, or SDP service names.
- Do not change HCI initialization command submission behavior in this change.

## Implementation

Update `BTSTACK_PROPERTY_DEFAULT_NAME` in
`btstack/adapter/btstack_property.c`. The GPCV2178 storage backend currently
does not restore a persisted Bluetooth name, so initialization will select this
default on every boot. Existing AT setters continue to update the in-memory
Classic/BLE name through the property APIs.

## Verification

- Add a contract test that requires the exact default name.
- Verify the test fails before the production change and passes afterward.
- Run all focused BTstack contract tests.
- Rebuild `libxstack.a` and the main ELF/BIN.
- Confirm the final library and ELF contain `GP-BT-ATBM6165` and no longer
  contain `BTstack Adapter`.
