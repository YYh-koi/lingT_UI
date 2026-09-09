# Minimal Customer SDK Package Design

## Goal

Generate a sibling customer SDK that contains only the files required to
rebuild the current main-project Debug configuration. The wireless delivery
remains binary-only: ATBM WiFi and BTstack may contain required headers and
their static archives, but no wireless implementation `.c` files.

The output directory is:

`D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`

## Baseline

The source of truth is the current Debug target in:

`Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj`

The baseline currently builds 309 non-wireless compilation units and links
20 explicit static libraries, including `libatbm_wifi.a` and `libbtstack.a`.
The dedicated internal wireless library projects are not customer artifacts.

## Selected Approach

Use a build-derived dependency closure instead of copying the SDK and then
maintaining a manual exclusion list.

The packager will:

1. Read the current Debug target, active project units, compiler options,
   include directories, linker script, and explicit linked archives.
2. Start from the active compilable project units.
3. Run dependency generation with the same Generalplus GCC 4.9.3 ABI,
   preprocessor definitions, forced includes, and include search paths.
4. Resolve the transitive project-local header closure.
5. Copy only the resulting sources and headers, the main G+ project,
   required project support files, the linker script, and the 20 linked
   archives.
6. Replace the two staged wireless archives with stripped customer copies
   and rebuild their archive indexes.
7. Generate `SHA256SUMS.txt` after all staged files reach their final form.

This approach intentionally supports the current Debug product, not arbitrary
future SDK modules that are absent from the current build.

## Retention Rules

The minimal package retains:

- Every existing active `.c`, `.cpp`, `.s`, and `.S` unit compiled by the
  current Debug project, excluding ATBM WiFi and BTstack implementation
  sources.
- Every project-local header reached by dependency generation from those
  active units.
- Forced-include configuration headers required by the current build.
- The main `.gpcmprj`, its linker script, startup unit, and any directly
  referenced project configuration file required to open or build it.
- Exactly the explicit static archives used by the Debug linker target.
- `lib\libatbm_wifi.a` and `lib\libbtstack.a`, stripped only in the customer
  stage.
- `SHA256SUMS.txt`.

Paths are preserved relative to the SDK root so existing project references
and include paths remain valid.

## Removal Rules

The minimal package removes:

- `Project\WirelessLibraries` and both internal library-building projects.
- `tools`, `patches`, internal design documents, verification output, tests,
  backups, temporary files, and source-control metadata.
- Directories with no retained file after dependency closure.
- Headers not reached from the current Debug compilation roots.
- Static archives not explicitly linked by the current Debug target.
- WiFi and BTstack implementation `.c` files.
- Object files, dependency files, IDE temporary output, and prior binaries
  other than required linked archives.

The main integrated project itself is retained in its current library-linked
form, so the customer package does not need the integration patch.

## Dependency Resolution

Dependency discovery must use the customer-visible source model:

- Non-wireless project compilation units are treated as source roots.
- Commented wireless units in the main project are not roots.
- The staged wireless headers may participate in dependency resolution, but
  wireless `.c` files may never be copied.
- Project-local dependencies are normalized to workspace-relative paths.
- Compiler/toolchain system headers are not copied into the SDK.
- An unresolved project-local include is a packaging error.

The dependency list must be deterministic and sorted before copying.

## Safety and Replacement

The packager generates into a new temporary sibling directory. It never
prunes the current customer directory in place.

Only after all validation gates pass:

1. Move the current customer directory to a timestamped backup.
2. Rename the validated temporary directory to the official `_customer`
   path.

This keeps the last valid package recoverable if generation or verification
fails.

## Validation Gates

The temporary package must pass all of the following before replacement:

- All retained project paths exist.
- No ATBM WiFi or BTstack implementation `.c` file exists.
- No internal wireless library project exists.
- Every retained header belongs to the computed dependency closure or is a
  required forced-include configuration header.
- Exactly the Debug target's explicit archives are present.
- WiFi archive member count is 92; BTstack archive member count is 59.
- Customer wireless archives contain no `.debug_*` sections and have valid
  archive indexes.
- The current Debug non-wireless source set compiles using only paths inside
  the temporary customer package.
- The resulting objects link successfully using only the temporary package's
  linker script and archives.
- Required wireless entry symbols are defined in the linked ELF.
- Every `SHA256SUMS.txt` entry verifies with zero failures.

## Deliverable

The customer compresses only:

`GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`

Timestamped `_backup_` or temporary `_candidate_` sibling directories are not
part of the deliverable.
