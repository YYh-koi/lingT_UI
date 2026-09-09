# Dedicated WiFi and Bluetooth Library Projects Design

## Goal

Create two independent G+ IDE static-library projects that build the current
ATBM WiFi driver and BTstack protocol stack. These projects become the primary
internal build path for `libatbm_wifi.a` and `libbtstack.a`. The customer
delivery continues to contain wireless headers and the two `.a` files, without
wireless implementation source.

## Project Layout

The projects are stored under one internal build area:

```text
Project/WirelessLibraries/
├── ATBM_WiFi_Library/
│   └── ATBM_WiFi_Library.gpcmprj
└── BTstack_Library/
    └── BTstack_Library.gpcmprj
```

Each directory has its own `obj/Debug` output tree. Object files are never
shared between the two library projects or with the application project.

## G+ Project Type and Outputs

Both projects use the SDK's existing native static-library project format:

```xml
<Option type="0" />
```

Only a `Debug` target is defined because this delivery is explicitly based on
the current complete-wireless Debug configuration.

Automatic library prefix and extension handling remains enabled:

```xml
prefix_auto="1" extension_auto="1"
```

The WiFi target output basename is `atbm_wifi`, producing:

```text
lib/libatbm_wifi.a
```

The Bluetooth target output basename is `btstack`, producing:

```text
lib/libbtstack.a
```

The WiFi project uses relative output path `../../../lib/atbm_wifi`; the
Bluetooth project uses `../../../lib/btstack`.

## Source Ownership

`ATBM_WiFi_Library.gpcmprj` contains exactly the 92 ATBM C units currently
selected by the main project under:

```text
gplib/liba_wifi_src/atbm/atbm6165
```

`BTstack_Library.gpcmprj` contains exactly the 57 BTstack C units currently
selected by the main project under:

```text
btstack
```

This includes the GPCV2178 BTstack port, controller transport, run loop,
storage, and enabled profiles.

The approved `tools/wireless_lib/wireless_sources.json` manifest is the
source-of-truth for validating project membership. Project validation fails
for a missing source, duplicate unit, unexpected unit, or source-count
divergence.

## Debug Configuration

Both projects use Generalplus ARM Embedded GCC 4.9.3 with the same active
compiler options and include search paths as the main application's current
Debug target. Required ABI and configuration options include:

```text
-O2
-g
-mcpu=cortex-a7
-mfpu=neon-vfpv4
-mfloat-abi=softfp
-fdata-sections
-ffunction-sections
-DWIFI_VENDOR=3
-DATBM_GENERALPLUS_SDIO=1
-DATBM_SDIO_PROBE_ONLY=0
-DATBM_SDIO_FW_ONLY=0
-D__CS_COMPILER__
-DGP_FREERTOS
```

Include directories are expressed relative to each dedicated project so the
projects remain relocatable with the SDK root.

The BTstack project additionally forces the GPCV2178 target configuration:

```text
-include ../../../btstack/ports/gpcv2178/btstack_config.h
```

This prevents a generic adapter configuration from occupying the shared
include guard before the target configuration is visible.

## Application Integration

The main project does not compile the 149 wireless source units. Its Debug
linker continues to consume:

```xml
<Add library="..\..\lib\libatbm_wifi.a" />
<Add library="..\..\lib\libbtstack.a" />
```

Wireless include paths and public compile definitions remain available to
application code. The library order remains WiFi first, BTstack second, after
the existing SDK libraries.

The existing-file modification continues to be delivered as:

```text
patches/wireless_static_library_integration.patch
```

The two dedicated `.gpcmprj` files are new files and therefore do not require
an existing-file patch.

## Build Workflow

Internal developers build the libraries in this order:

1. Open and build the `Debug` target of `ATBM_WiFi_Library.gpcmprj`.
2. Open and build the `Debug` target of `BTstack_Library.gpcmprj`.
3. Build or link the main application's `Debug` target.

The existing PowerShell builder remains an internal reproducibility and
cross-check tool. It is no longer described as the primary build mechanism.

## Customer Delivery

The dedicated library projects and wireless C sources stay in the complete
internal SDK. The customer staging process delivers:

- `lib/libatbm_wifi.a`;
- `lib/libbtstack.a`;
- required ATBM WiFi and BTstack headers with compatible paths;
- the application integration project and patch;
- integration instructions and SHA-256 manifest.

The staged ATBM WiFi and BTstack trees contain no `.c` files.

## Validation

Completion requires:

1. Both `.gpcmprj` files parse as valid XML.
2. Both projects declare `type="0"` and only the `Debug` target.
3. The WiFi project contains exactly the manifest's 92 WiFi units.
4. The BTstack project contains exactly the manifest's 57 Bluetooth units.
5. Compiler options, include paths, CPU/FPU ABI, and GCC version match the
   current main Debug baseline.
6. Building each dedicated project produces its expected archive in `lib`.
7. Archive member counts are 92 and 57, and archive indexes are readable.
8. The main application links successfully with both generated archives.
9. Required WiFi and Bluetooth entry points are defined in the final ELF.
10. The customer stage contains zero ATBM WiFi or BTstack `.c` files and all
    checksum entries verify.

## Failure Handling

A library build must stop on a missing source, compile error, archive error, or
output-path mismatch. Validation reports the project and source responsible.
An archive is not copied into a customer package unless both dedicated library
projects and the main integration link have passed.

## Non-Goals

- Creating Release variants with different optimization or ABI settings.
- Combining WiFi and Bluetooth in one project or one archive.
- Changing wireless behavior or public APIs.
- Giving wireless implementation source or internal library projects to the
  customer.
