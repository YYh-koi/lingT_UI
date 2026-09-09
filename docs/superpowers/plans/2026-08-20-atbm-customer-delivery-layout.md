# ATBM Customer Delivery Layout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan.

**Goal:** Relocate the customer-delivered network sources to `gplib/liba_wifi_src/atbm/network`, compile them only in the ATBM main project, and keep `libatbm_wifi.a` free of network source units.

**Architecture:** The ATBM package has three customer-facing parts: `atbm/network`, `libatbm_wifi.a`, and `atbm_wifi.h`. The WiFi library project may include network headers required by driver sources, while the ATBM main project owns lwIP, DHCP, and port source compilation. The 8189FTV project remains RTK-based and independent.

**Tech Stack:** PowerShell contract tests, Generalplus `.gpcmprj` projects, `gpcmprj2make.exe`, GNU Make, ARM GCC.

---

## Task 1: Add the new delivery-layout contract (RED)

- [x] Update `tools/wireless_lib/tests/test_atbm_rtk_isolation.ps1` to require `gplib/liba_wifi_src/atbm/network` and reject the former nested network directory.
- [x] Assert that the ATBM library project references the new network include paths but contains no network `.c` unit.
- [x] Assert that the ATBM main project compiles network sources from the new location and contains no old network path.
- [x] Assert that the AP default IP remains `192.168.42.1`.
- [x] Run:

  ```powershell
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\wireless_lib\tests\test_atbm_rtk_isolation.ps1
  ```

  Expected: FAIL because `gplib/liba_wifi_src/atbm/network` does not exist yet.

## Task 2: Relocate the network source tree (GREEN)

- [x] Resolve and verify the exact source and destination paths under `gplib/liba_wifi_src/atbm`.
- [x] Move only `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network` to `gplib/liba_wifi_src/atbm/network` with `Move-Item -LiteralPath`.
- [x] Verify the new root exists and the old root no longer exists.

## Task 3: Update active build metadata

- [x] Replace the old network path with the new path in:

  - `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
  - `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
  - `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [x] Confirm `ATBM_WiFi_Library.gpcmprj` has no source unit under `atbm/network`.
- [x] Confirm `GPCV2178_turnkey_sdk_release.gpcmprj` still owns the network source units.
- [x] Regenerate the corresponding Debug makefiles with `gpcmprj2make.exe --local` instead of hand-editing generated makefiles.

## Task 4: Verify the contract (GREEN)

- [x] Run `test_atbm_rtk_isolation.ps1` and confirm PASS.
- [x] Run all PowerShell tests under `tools/wireless_lib/tests` and confirm PASS.
- [x] Search active source/build metadata for the old network path and confirm no matches outside historical documentation and build outputs.

## Task 5: Build and inspect ATBM artifacts

- [x] Run `before_debug`, then force-build `ATBM_WiFi_Library_Debug.mk` with `-B -j4`.
- [x] Inspect `libatbm_wifi.a` members and confirm it contains no `network`, lwIP, DHCP, or `atbm_lwip_port` source object.
- [x] Run `before_debug`, then force-build `GPCV2178_turnkey_sdk_release_Debug.mk` with `-B -j4`.
- [x] Confirm the ATBM main ELF is generated successfully.

## Task 6: Prove RTK independence and preserve 8189FTV

- [x] In a `try/finally`, rename only `gplib/liba_wifi_src/rtk` to a validated temporary sibling name.
- [x] Force-build the ATBM WiFi library and ATBM main project while RTK is absent.
- [x] Restore the RTK directory in `finally` and verify restoration.
- [x] Regenerate and force-build the 8189FTV Debug project after RTK restoration.

## Task 7: Final delivery audit

- [x] Verify the customer-facing artifacts exist: `atbm/network`, `libatbm_wifi.a`, and `atbm_wifi.h`.
- [x] Verify `192.168.42.1` is unchanged.
- [x] Report changed files, build evidence, and any remaining hardware-only verification risk.

No commit steps are included because this workspace has no Git or SVN working-copy metadata.
