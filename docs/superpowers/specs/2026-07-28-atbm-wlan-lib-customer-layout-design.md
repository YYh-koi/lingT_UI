# ATBM `wlan_lib` Customer Layout Design

## Goal

Make the ATBM customer-facing layout match the existing 8189FTV convention
while preserving the approved two-library architecture:

```text
gplib/liba_wifi_src/atbm/atbm6165/
|-- api/
|   |-- atbm_wifi_types.h
|   |-- atbm_wifi_driver_api.h
|   `-- atbm_api.h
`-- wlan_lib/
    |-- libatbm_wifi.a
    `-- libbtstack.a
```

The internal SDK continues to contain the complete WiFi and BTstack sources
and the two independent G+ static-library projects.

## Decisions

- `wlan_lib` is the canonical binary output directory for both independent
  G+ library projects.
- The main Debug application project links the two archives from
  `atbm6165/wlan_lib`, not from the SDK-root `lib` directory.
- The customer package exposes only `api` and `wlan_lib` below `atbm6165`.
- `atbm_wifi_driver_api.h` and `atbm_api.h` must compile without including
  `atbm_hal.h` or other driver-internal headers.
- A new `atbm_wifi_types.h` owns the public ABI types, public constants, and
  opaque forward declarations needed by both public headers.
- `Project/wifi_demo/atbm_wifi_probe_demo.c` includes public API headers only.
- The package builder strips debug sections and reindexes both staged
  archives in their new `wlan_lib` location.
- Modified pre-existing files are represented in
  `patches/wireless_static_library_integration.patch`; new files are shipped
  as normal files.

## Compatibility

Public integer typedefs retain the existing ARM ABI widths. The public
`ATBM_NETIF` type remains an alias for `struct netif`, and private structures
remain opaque. `WLAN_MACLIST` retains the existing fixed capacity of eight
stations.

## Verification

- Contract tests require both G+ projects and the main project to use the new
  archive paths.
- A compiler smoke test includes each public header without `atbm_hal.h`.
- The packaging test requires `api` plus `wlan_lib` and rejects other
  `atbm6165` customer subdirectories.
- Both G+ project-contract tests and tool-contract tests pass serially.
- The generated customer SDK compiles and links using only staged files.
- The final patch applies to the captured baseline and reproduces modified
  existing files.
