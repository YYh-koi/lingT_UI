# ATBM Private SDIO L2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the ATBM6165 WiFi archive with an ATBM-owned Generalplus SDIO L2 while leaving the existing 8189FTV implementation unchanged and keeping the customer delivery to `atbm_wifi.h` plus `libatbm_wifi.a`.

**Architecture:** Copy the six active RTK-derived SDIO implementation files into an ATBM-private directory, rename all link-visible symbols with the `atbm_gp_` prefix, and replace the sole driver_l2 clock dependency with an equivalent driver_l1 clock helper. The dedicated ATBM library project compiles the three new C units; the complete SDK keeps the original RTK units for 8189FTV.

**Tech Stack:** Generalplus G+ IDE project (`.gpcmprj`), GNU Arm static archive, C, PowerShell verification scripts.

---

## Task 1: Add a Failing Structural Contract Test

- [ ] Create `tools/wireless_lib/tests/test_atbm_private_sdio_l2.ps1`.
- [ ] Assert the six ATBM-private SDIO files exist.
- [ ] Assert the WiFi library project contains the three private C units and no RTK SDIO include path.
- [ ] Assert the manifest expects 96 WiFi sources and lists the three private C units.
- [ ] Assert the original six 8189FTV SDIO files keep their baseline SHA-256 hashes.
- [ ] Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_private_sdio_l2.ps1
```

Expected: failure because the private files and project entries do not exist yet.

## Task 2: Add the ATBM-Private SDIO L2

- [ ] Add:

```text
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/core/atbm_gp_sdio_irq.h
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_card.h
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_l2.h
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_cis.c
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_l2.c
```

- [ ] Preserve transfer, IRQ, queue, and diagnostics behavior from the current implementation.
- [ ] Rename externally visible SDIO functions, handlers, and shared state with `atbm_gp_`.
- [ ] Replace `drvl2_sdc_clock_gate_enable()` with a private helper based on `drv_l1_clock_set_system_clk_en()`.
- [ ] Confirm the private sources contain no RTK SDIO include path and no `drv_l2_sdc.h`.

## Task 3: Switch the ATBM Port and Library Project

- [ ] Update `gplib/liba_wifi_src/atbm/atbm6165/os/freertos/atbm_os_sdio_gp.c` to include and call only the ATBM-private SDIO API.
- [ ] Update `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c` to initialize the ATBM-private SDIO implementation.
- [ ] Add the three C units and private include directories to `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`.
- [ ] Remove the dedicated ATBM library project's RTK SDIO include directories.
- [ ] Update `tools/wireless_lib/wireless_sources.json` from 93 to 96 WiFi sources.
- [ ] Update tool contract test counts for the three new archive members.

## Task 4: Verify the Archive and Compatibility

- [ ] Run the private SDIO structural test and the existing wireless library tool tests.
- [ ] Build the dedicated ATBM WiFi Debug archive.
- [ ] Inspect `libatbm_wifi.a` for the three new members and required `atbm_gp_sdio_*` definitions.
- [ ] Confirm the archive has no unresolved RTK SDIO or `drvl2_sdc_clock_gate_enable` symbols.
- [ ] Confirm the original 8189FTV SDIO hashes are unchanged.
- [ ] Link or build the complete project when the available CLI/toolchain supports it.

## Task 5: Package and Patch

- [ ] Synchronize the rebuilt `libatbm_wifi.a` and the unchanged public `atbm_wifi.h` into `GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer`.
- [ ] Confirm the customer package contains no ATBM-private SDIO source/header files.
- [ ] Generate `patches/atbm_private_sdio_l2.patch`, including all new files and modifications to existing files.
- [ ] Re-run the focused verification commands after patch creation.

Because this workspace is not a Git worktree, successful tests and the generated patch serve as the implementation checkpoints instead of commits.
