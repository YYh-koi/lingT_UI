# BTstack Feature Configuration Design

## Goal

Use one canonical `btstack_config.h` to select stack families and individual Profiles. Generic Classic and LE switches control their family-level behavior, while each Profile has its own explicit switch. Platform-specific hardware and RTOS settings remain outside the canonical protocol configuration.

## Configuration Files

The repository contains exactly one file named `btstack_config.h`:

```text
btstack/include/btstack_config.h
```

It owns protocol-family selection, Profile selection, bonding selection, and platform-neutral capacities.

The existing GPCV2178 configuration is renamed to:

```text
btstack/ports/gpcv2178/btstack_gpcv2178_config.h
```

The GPCV2178 file owns only target-specific settings such as transport, UART, H5, logging, task sizes, task priorities, and platform logging bindings. It includes the canonical `btstack_config.h`. GPCV2178 build projects force-include `btstack_gpcv2178_config.h` instead of a second `btstack_config.h`.

The old files are removed:

```text
btstack/include/adapter/btstack_config.h
btstack/ports/gpcv2178/btstack_config.h
```

## Canonical Feature Macros

Core and family selection:

```c
#define BTSTACK_CORE    1
#define BTSTACK_CLASSIC 1
#define BTSTACK_LE      1
#define BTSTACK_BONDING 0
```

Classic Profiles:

```c
#define BTSTACK_A2DP_SINK        1
#define BTSTACK_HFP_HF           1
#define BTSTACK_AVRCP_CONTROLLER 1
#define BTSTACK_AVRCP_TARGET     1
#define BTSTACK_PBAP_CLIENT      1
#define BTSTACK_HID_DEVICE       0
```

LE Profiles:

```c
#define BTSTACK_GATT_CLIENT 0
#define BTSTACK_GATT_SERVER 0
#define BTSTACK_HOGP_DEVICE 0
```

The canonical file continues to own platform-neutral capacities such as event lengths, local-name length, advertising-data length, HID/HOGP descriptor limits, HFP text limits, and GATT limits.

## Parent and Child Semantics

A Profile is active only when both its family and Profile switches are nonzero:

```c
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_TARGET
#if BTSTACK_CLASSIC && BTSTACK_PBAP_CLIENT
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE

#if BTSTACK_LE && BTSTACK_GATT_CLIENT
#if BTSTACK_LE && BTSTACK_GATT_SERVER
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
```

Turning off a family switch disables all of its children even if child macros remain set to `1`. This is intentional and does not produce a configuration error; it permits one-line family shutdown.

AVRCP shared session code is active when either role is selected:

```c
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
```

The Controller and Target role files are guarded independently. When only one role is enabled, the shared session initializes and calls only that selected role.

## LE Family Behavior

`BTSTACK_LE` alone controls generic LE capabilities:

- LE GAP initialization
- advertising and scan-response data handling
- scanning
- Central and Peripheral upstream support
- LE privacy support

These generic capabilities do not receive separate public feature macros.

GATT and HOGP remain separately selectable Profiles. The existing `btstack_gatt.c` API represents local database, notification, and indication behavior and is therefore controlled by `BTSTACK_GATT_SERVER`. `BTSTACK_GATT_CLIENT` is reserved for a future remote discovery/read/write/subscription facade; this change does not invent that API.

## Upstream BTstack Compatibility

All project-owned `BTSTACK_ENABLE_*` feature macros are removed without aliases, including the old family, Profile, and bonding names.

The upstream BTstack library still requires its native presence macros such as:

```c
ENABLE_CLASSIC
ENABLE_BLE
ENABLE_LE_CENTRAL
ENABLE_LE_PERIPHERAL
```

The canonical configuration derives these upstream macros from `BTSTACK_CLASSIC` and `BTSTACK_LE`. They remain upstream integration details and are not used by project Profile code to select individual Profiles.

## Runtime and ABI Behavior

Disabled Profile public functions remain linkable and return their established unsupported/default result:

- status-returning functions return `BTSTACK_STATUS_NOT_SUPPORTED`;
- boolean queries return `false`;
- handles or counts return zero;
- state queries return their idle/default state.

This preserves the public source/link interface across feature configurations. Disabled Profile implementations must not initialize native handlers, register SDP services, allocate static Profile state, or participate in Profile Manager linkage.

The Profile Manager initializes and subscribes only enabled Profiles. Cross-Profile behavior is compiled only when every participating Profile is enabled; for example, A2DP-triggered AVRCP connection requires A2DP and at least one usable AVRCP role, and HFP-to-PBAP cleanup requires both HFP HF and PBAP Client.

## Build Integration

API, public AT, and Legacy AT remain mutually selected builds. Their project files and generated makefiles force-include the renamed GPCV2178 platform configuration, which in turn includes the canonical feature configuration.

Host tests include the canonical file directly and do not need `project.h`, FreeRTOS, or GPCV2178 logging headers.

Source manifests may continue to contain all public Profile translation units so ABI stubs remain available. Compile-time guards remove disabled native behavior and Profile storage.

## Verification

Tests must establish:

1. Exactly one `btstack_config.h` exists under `btstack/`.
2. No project-owned `BTSTACK_ENABLE_*` macro remains in production code, public headers, project files, or generated build files.
3. The renamed GPCV2178 configuration contains only platform concerns and includes the canonical configuration.
4. Macro compile matrices cover family off, individual child off, AVRCP CT-only, AVRCP TG-only, AVRCP both, GATT Server-only, and all currently selected Profiles.
5. Disabled Profile APIs preserve their unsupported/default ABI behavior.
6. Profile Manager setup and cross-Profile linkage follow the parent-and-child conditions.
7. LE off disables generic advertising, scanning, Central, Peripheral, and privacy paths.
8. Official API, AT, Legacy, and main firmware builds regenerate successfully with no new warnings from the configuration migration.

Target-board interoperability remains a separate validation concern; compile and host contracts do not prove radio behavior.
