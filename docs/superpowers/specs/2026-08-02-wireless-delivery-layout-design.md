# Wireless Delivery Layout Design

## Goal

Use the new `atbm6165/bt` directory as the canonical Bluetooth delivery
location, and keep `atbm6165/wlan_lib` as the canonical WiFi delivery
location. Preserve the original source headers in their existing source trees
for internal compilation.

## Canonical customer layout

```text
gplib/liba_wifi_src/atbm/atbm6165/
|-- bt/
|   |-- libxstack.a
|   `-- bt_xstack.h
`-- wlan_lib/
    |-- libatbm_wifi.a
    `-- atbm_wifi.h
```

Only the application-facing headers are published. Internal BTstack and ATBM
headers are not copied into these delivery directories.

## Header sources

- Publish `btstack/include/adapter/bt_xstack.h` as
  `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`.
- Publish `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h` as
  `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`.
- Keep both original source headers in place. Internal library builds continue
  to compile against their existing source include trees.

The application-facing headers are self-contained and may include only system
headers or declarations defined in the same file.

## Build and application integration

- The BTstack G+IDE library project outputs
  `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`.
- The ATBM WiFi project continues to output
  `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`.
- The main Debug target links the two archives from their respective canonical
  directories.
- The main project exposes both delivery directories as include paths.
- The Bluetooth demo includes `bt_xstack.h`; the WiFi demo includes
  `atbm_wifi.h`.

## Command-line build and packaging

The PowerShell wireless builder publishes each archive and application header
to its canonical directory. The customer-package staging logic copies exactly
the following wireless deliverables:

```text
bt/libxstack.a
bt/bt_xstack.h
wlan_lib/libatbm_wifi.a
wlan_lib/atbm_wifi.h
```

The package must not contain `wlan_lib/libxstack.a`, internal BTstack headers,
or private ATBM headers. Strip, ranlib, checksum, project-generation, delivery
patch, and documentation logic must use the new paths.

## Error handling

Build and packaging commands fail if a required archive or application header
is missing. Contract tests reject duplicate archives, legacy Bluetooth archive
paths, or additional published wireless headers.

## Verification

1. Contract tests first fail under the old two-archives-in-`wlan_lib` layout.
2. Build `libatbm_wifi.a` and `libxstack.a` into their canonical directories.
3. Verify `libxstack.a` exports `bt_xstack_init` and `bt_xstack_send_at`.
4. Build the main Debug firmware and verify its ELF contains
   `bt_xstack_demo_start`, `bt_xstack_init`, and `bt_xstack_send_at`.
5. Regenerate the customer SDK and verify the exact four-file wireless
   delivery contract and checksum entries.
6. Run the Bluetooth, demo, wireless-library, and packaging regression tests.

Hardware UART/H5 operation remains a board-level verification step and is not
proven by the static build and packaging checks.
