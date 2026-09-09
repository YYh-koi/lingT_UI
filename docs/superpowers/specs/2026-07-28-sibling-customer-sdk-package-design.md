# Sibling Customer SDK Package Design

## Goal

Generate a customer-ready SDK directory next to the complete internal SDK so
the whole customer directory can be compressed without exposing ATBM WiFi or
BTstack implementation source.

## Directory Layout

The complete internal SDK remains unchanged at:

```text
D:\Project\Platform\Generalplus\
  GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm
```

The packaging tool generates:

```text
D:\Project\Platform\Generalplus\
  GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer
```

The `_customer` directory is the folder that can be compressed and delivered
in its entirety.

## Internal SDK Boundary

The internal SDK retains:

- all ATBM WiFi implementation source;
- all BTstack implementation source;
- `ATBM_WiFi_Library.gpcmprj`;
- `BTstack_Library.gpcmprj`;
- object files, unstripped libraries, build logs, and verification data.

No source is physically moved out of the internal SDK. This preserves relative
include paths and reproducible library builds.

## Customer SDK Contents

The customer directory preserves the SDK layout needed by the application
project. It contains:

- all non-wireless SDK files required to build the application;
- required ATBM WiFi headers under their compatible paths;
- required BTstack headers under their compatible paths;
- `lib/libatbm_wifi.a`;
- `lib/libbtstack.a`;
- the application project already configured to link both libraries;
- the application integration patch and instructions;
- `SHA256SUMS.txt`.

## Exclusions

The customer directory excludes:

- every `.c` file below the ATBM `atbm6165` implementation tree;
- every `.c` file below the BTstack tree;
- the duplicate top-level `atbm_rtos_wifi6` source tree;
- `Project/WirelessLibraries`, because those internal projects cannot build
  without the private wireless sources;
- internal `output`, `tmp`, `.codebase-memory`, `prjDebug`, tests, backups,
  object files, dependency files, and source-control metadata.

Other non-wireless SDK source files remain present.

## Output Safety

The packager refuses to write into a non-empty customer directory. It never
deletes or replaces an existing package implicitly. To generate a new package,
the caller supplies a new directory name or explicitly archives and removes
the previous customer directory first.

The destination must not equal the internal SDK root or be a parent of it.

## Packaging Entry Point

The primary command is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/package_wireless_release.ps1
```

Its default destination changes from the internal
`output/customer_wireless_sdk` directory to the sibling `_customer` directory.
`-StageRoot` remains available for an explicit alternative destination.

## Verification

Packaging succeeds only when:

1. Both customer archives exist.
2. The staged ATBM WiFi tree contains zero `.c` files.
3. The staged BTstack tree contains zero `.c` files.
4. `Project/WirelessLibraries` is absent.
5. The duplicate top-level `atbm_rtos_wifi6` tree is absent.
6. The application project has zero active wireless C units and links each
   archive exactly once.
7. Every `SHA256SUMS.txt` entry matches the delivered file's length and hash.
8. A customer-form application compile and link succeeds using only the
   sibling package.

## Patch Policy

The existing application project remains the only pre-existing SDK text file
modified for library integration, and that modification remains represented by
`patches/wireless_static_library_integration.patch`. Packaging tools,
dedicated library projects, tests, and documentation are new task files.
