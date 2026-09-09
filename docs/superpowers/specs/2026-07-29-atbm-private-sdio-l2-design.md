# ATBM Private Generalplus SDIO L2 Design

## Goal

Give ATBM6165 its own Generalplus SDIO L2 implementation so future ATBM SDIO
changes do not modify or reuse implementation files under
`gplib/liba_wifi_src/rtk/8189FTV`.

The existing 8189FTV source and build path must remain available and unchanged.
The customer-facing WiFi delivery remains one public header and one archive:

```text
atbm_wifi.h
libatbm_wifi.a
```

## Confirmed Constraints

- Preserve the current working ATBM6165 Debug behavior as the baseline.
- Do not modify the existing RTK/8189FTV SDIO source or project membership.
- Keep 8189FTV buildable in the complete internal SDK.
- ATBM SDIO must not reference RTK-owned headers, source files, or symbols.
- At the driver hierarchy level, ATBM SDIO L2 may depend only on `driver_l1`.
- FreeRTOS/CMSIS-RTOS, Generalplus common types, allocation, logging, and board
  configuration remain permitted platform-service dependencies.
- Existing-file changes must be delivered in a patch.

## Architecture

The resulting ATBM path is:

```text
ATBM WiFi driver
    |
    v
ATBM private Generalplus SDIO L2
    |
    v
driver_l1/sdc + driver_l1/clock + driver_l1/interrupt + driver_l1/gpio
    |
    v
GPCV2178 SDIO controller
```

The RTK path remains:

```text
8189FTV
    |
    v
RTK-owned Generalplus SDIO implementation
    |
    v
driver_l1
```

The two L2 implementations coexist in one SDK but expose different linker
symbols. Application startup continues to select the active WiFi vendor through
`WIFI_VENDOR`; it must not initialize both controllers on the same SDIO device.

## Source Layout

Add an ATBM-owned private platform directory:

```text
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/
|-- core/
|   `-- atbm_gp_sdio_irq.h
|-- include/
|   |-- atbm_gp_sdio_card.h
|   `-- atbm_gp_sdio_l2.h
`-- source/
    |-- atbm_gp_sdio_card.c
    |-- atbm_gp_sdio_cis.c
    `-- atbm_gp_sdio_l2.c
```

The initial implementations are behavior-preserving copies of the six active
files currently used from `8189FTV/platform/sdio`:

```text
core/sdio_irq.h
include/card.h
include/drv_l2_sdio_rtk.h
source/card.c
source/drv_l2_sdio_cis_rtk.c
source/drv_l2_sdio_rtk.c
```

`wifi_io.c` and `wifi_io.h` are not copied. The active ATBM SDIO call path does
not use their functions, and `wifi_io.c` depends on RTK `osdep_service.h`.

## Symbol Isolation

All externally visible copied functions and globals use an `atbm_gp_` prefix.
Examples:

```text
drvl2_sdio_init           -> atbm_gp_sdio_init
drvl2_sdio_deinit         -> atbm_gp_sdio_deinit
drvl2_sdio_rw_direct      -> atbm_gp_sdio_rw_direct
drvl2_sdio_irq_en         -> atbm_gp_sdio_irq_en
sdio_claim_irq            -> atbm_gp_sdio_claim_irq
sdio_release_irq          -> atbm_gp_sdio_release_irq
sdio_claim_host           -> atbm_gp_sdio_claim_host
sdio_release_host         -> atbm_gp_sdio_release_host
SD0_IRQHandler            -> atbm_gp_sdio0_irq_handler
SD1_IRQHandler            -> atbm_gp_sdio1_irq_handler
```

Private state that does not need cross-file visibility becomes `static`.
Remaining cross-file state is also prefixed. Private header include guards are
unique to ATBM.

The ATBM L2 initializer registers `atbm_gp_sdio0_irq_handler` or
`atbm_gp_sdio1_irq_handler` with `vic_irq_register()`. It does not rely on or
replace the RTK `SD0_IRQHandler` and `SD1_IRQHandler` definitions.

## Driver Dependencies

The current RTK-derived L2 calls `drvl2_sdc_clock_gate_enable()` during init and
deinit. That is its only functional dependency on another `driver_l2` module.

The ATBM private implementation replaces it with:

```text
atbm_gp_sdio_clock_gate_enable()
```

This private helper preserves the existing clock-register behavior using:

```text
drv_l1_clock_set_system_clk_en()
```

After this replacement:

- no ATBM source includes `drv_l2_sdc.h`;
- no ATBM source includes an RTK SDIO header;
- no ATBM archive member has an undefined `drvl2_sdc_*`,
  RTK `drvl2_sdio_*`, or RTK `sdio_*` dependency;
- all hardware-driver dependencies terminate at `driver_l1`.

Permitted non-driver dependencies include CMSIS-RTOS/FreeRTOS queue, mutex, and
thread APIs; Generalplus allocation and printing services; C library routines;
board configuration; and SDK integer types.

## ATBM Integration

Update the ATBM port to call only the private namespaced API:

```text
gplib/liba_wifi_src/atbm/atbm6165/os/freertos/atbm_os_sdio_gp.c
gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c
```

The ATBM library project:

```text
Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj
```

adds the three new C units and their private include paths. It removes its
compile-time include dependency on `8189FTV/platform/sdio`.

The new private L2 objects are compiled into:

```text
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
```

No additional customer library or public header is introduced.

## RTK Compatibility

The following remain unchanged:

- all files below `gplib/liba_wifi_src/rtk/8189FTV`;
- `lib8189FTV_WLAN.a`;
- RTK project source membership and include paths;
- existing RTK SDIO symbols and interrupt handler names.

The main project may continue compiling the RTK SDIO platform units. ATBM uses
its prefixed implementation from `libatbm_wifi.a`, so the two sets do not
collide or resolve each other's dependencies.

## Customer Packaging

The complete internal SDK contains the new ATBM private sources and library
project. The customer SDK continues to expose only:

```text
gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
```

The customer package contains no ATBM private SDIO `.c` or private `.h` files.
Generalplus `driver_l1` and common SDK runtime facilities remain customer-side
link dependencies, as before.

## Implementation Discipline

The extraction is completed before any behavioral SDIO fix:

1. Copy the current working implementation.
2. Change paths, include guards, symbols, and the one L2 clock dependency.
3. Rebuild and verify unchanged ATBM behavior.
4. Make future interrupt, queue, or transfer fixes only in the ATBM-owned copy.

This prevents the extraction from being confused with fixes for
`zero pending`, queue-full notifications, or other runtime diagnostics.

## Validation

Completion requires:

1. The dedicated ATBM WiFi Debug project builds `libatbm_wifi.a`.
2. The archive contains the three ATBM private SDIO objects.
3. The archive defines the required `atbm_gp_sdio_*` symbols.
4. The archive has no undefined references to RTK SDIO symbols or
   `drvl2_sdc_clock_gate_enable()`.
5. The ATBM project and archive dependency data contain no
   `8189FTV/platform/sdio` path.
6. The complete ATBM main project links without duplicate symbols.
7. The customer SDK links with the rebuilt archive and contains no ATBM
   implementation source.
8. ATBM6165 AP association, WPA handshake, DHCP assignment, sustained traffic,
   reconnect, and power-cycle tests pass.
9. The 8189FTV configuration still compiles and links using the untouched RTK
   implementation.
10. The final map attributes the active ATBM SDIO implementation to prefixed
    archive symbols, not RTK SDIO objects.

## Patch Delivery

All modifications to existing files are recorded in a dedicated patch:

```text
patches/atbm_private_sdio_l2.patch
```

The patch also records new ATBM private source and header files so the complete
change can be reviewed or reapplied as one unit.

## Non-Goals

- Copying or modifying `driver_l1`.
- Removing or refactoring the RTK/8189FTV implementation.
- Providing a separate `libatbm_sdio.a`.
- Changing the public `atbm_wifi.h` API.
- Fixing SDIO interrupt behavior during the extraction.
- Supporting simultaneous ATBM6165 and 8189FTV initialization on one SDIO
  controller.
