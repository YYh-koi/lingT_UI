# WiFi and Bluetooth Static Library Delivery Design

## Goal

Package the ATBM WiFi implementation and BTstack implementation as two ARM
static libraries for customer delivery. The customer package contains only the
required headers and `.a` libraries for these two modules; it does not contain
their C source files.

The integration must continue to build with the current
`GPCV2178_turnkey_sdk_release.gpcmprj` Debug configuration.

## Confirmed Inputs

- Target: GPCV2178, Cortex-A7.
- Toolchain: Generalplus ARM Embedded GCC 4.9.3, matching the `.comment`
  section in the current Debug object files.
- ABI and optimization baseline: the current Debug configuration, including
  `-O2`, `-mcpu=cortex-a7`, `-mfpu=neon-vfpv4`, and
  `-mfloat-abi=softfp`.
- WiFi scope: the 92 ATBM C units currently referenced under
  `gplib/liba_wifi_src/atbm/atbm6165` by the main project.
- Bluetooth scope: the 57 BTstack C units currently referenced under
  `btstack` by the main project, including the GPCV2178 port.
- Required outputs:
  - `libatbm_wifi.a`
  - `libbtstack.a`

Existing wireless object files are not used as the release input. They predate
the current sources and their object count does not match the current project
source count.

## Architecture

The internal SDK remains source-based so the libraries can be reproduced.
A dedicated wireless-library build consumes the same sources, include paths,
preprocessor definitions, CPU options, floating-point ABI, and optimization
options as the main Debug target.

The customer-facing integration replaces the 92 WiFi and 57 BTstack source
units in the main project with two linker inputs:

1. `libatbm_wifi.a`
2. `libbtstack.a`

The archives include the existing GPCV2178 OS, SDIO/UART, controller, storage,
and BTstack port implementations. These board-specific details are therefore
fixed to this SDK delivery. The libraries may still have normal unresolved
references to SDK services such as FreeRTOS, drivers, libc, and application
hooks; those references are resolved when the customer links the full SDK
application.

## Build Method

New build metadata or scripts will:

1. Derive the wireless source lists from the current main project or maintain
   validated explicit manifests that are checked against it.
2. Compile every wireless source from the current source tree into a dedicated
   object directory, rather than reusing `prjDebug/obj/Debug`.
3. Apply the current Debug compiler definitions and include paths.
4. Create deterministic archives with the matching GCC 4.9.3
   `arm-none-eabi-ar` and index them with `arm-none-eabi-ranlib`.
5. Keep unstripped internal archives for diagnosis.
6. Produce customer archive copies with debug information removed using
   `arm-none-eabi-strip --strip-debug`; exported and required link symbols
   remain intact.

The build must fail if a listed source is missing, compilation fails, archive
creation fails, or the source count diverges from the expected 92 WiFi and 57
BTstack units.

## Header Delivery

The customer package preserves the directory structure required by the existing
include paths. It copies the WiFi and BTstack headers needed to compile the SDK
and public application interfaces, while excluding:

- `.c` files;
- intermediate `.o` files;
- internal build output;
- tests, backups, code-graph data, and source-control metadata.

Header selection will be verified by building the customer-form integration
without access to the wireless source directories. If a transitive include is
required, the corresponding header is added to the package rather than exposing
its implementation source.

## Main Project Integration

`Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj` will be
updated so the Debug target:

- no longer compiles or links the 149 wireless C units;
- retains the include paths needed by customer application code;
- links `libatbm_wifi.a` and `libbtstack.a` in an order that resolves their
  dependencies;
- otherwise preserves the current Debug configuration.

If archive extraction exposes a cyclic dependency between the two libraries,
the linker configuration will use an archive group (`--start-group` and
`--end-group`) rather than merging the libraries.

## Patch Policy

Before any existing file is changed, an original copy is captured outside the
deliverable tree. A unified patch is generated for all modified existing text
files. The patch uses workspace-relative paths and is validated by applying it
to clean original copies.

New files are listed separately in the delivery manifest and are not
represented as modifications to pre-existing files.

## Customer Package

The generated package contains:

- `lib/libatbm_wifi.a`
- `lib/libbtstack.a`
- required WiFi and BTstack headers in their compatible directory structure;
- the patched main project file;
- the unified integration patch;
- a concise integration README;
- a manifest containing file sizes and SHA-256 checksums.

The package does not delete or rewrite the internal wireless source trees. It is
generated as a separate staging directory.

## Verification

Completion requires all of the following:

1. Compile all 92 WiFi and 57 BTstack units from current source with the Debug
   ABI/configuration baseline.
2. Confirm both archives are valid ARM ELF archives and have archive indexes.
3. Audit defined and undefined symbols with `arm-none-eabi-nm`.
4. Build and link the main Debug application after replacing direct wireless
   source compilation with the two archives.
5. Verify required WiFi and Bluetooth entry points remain present in the final
   ELF/map.
6. Build from the customer staging layout without relying on wireless C source
   directories.
7. Scan the customer package and confirm that it contains no WiFi or BTstack
   `.c` files.
8. Verify the generated patch applies cleanly to captured original files.
9. Generate and verify SHA-256 checksums for all delivered artifacts.

## Non-Goals

- Changing WiFi or Bluetooth behavior.
- Refactoring the wireless APIs.
- Supporting a different CPU, floating-point ABI, toolchain version, or project
  configuration.
- Combining WiFi and Bluetooth into one archive.
- Removing the internal source tree used to reproduce future libraries.
