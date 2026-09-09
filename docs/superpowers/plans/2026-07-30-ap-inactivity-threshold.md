# AP Inactivity Threshold Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore the AP firmware inactivity minimum threshold to the ATM630 reference value so an active phone is not removed shortly after association.

**Architecture:** Keep the firmware inactivity mechanism and event handling unchanged. Change only the configurable minimum threshold from `6` to `39`, retaining `max=1`, then verify the source value and rebuild the ATBM WiFi library used by the firmware.

**Tech Stack:** C preprocessor configuration, ATBM6165 WiFi driver, Generalplus SDK build.

---

### Task 1: Verify and change the inactivity threshold

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h:87`
- Test: PowerShell static configuration assertion

- [x] **Step 1: Run the failing configuration assertion**

```powershell
$text = Get-Content -Raw gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h
if ($text -notmatch '#define\s+ATBM_AP_INACTIVITY_MIN\s+\(39\)') { throw 'ATBM_AP_INACTIVITY_MIN is not 39' }
```

Expected: FAIL with `ATBM_AP_INACTIVITY_MIN is not 39`.

- [x] **Step 2: Apply the minimal source change**

```c
#ifndef ATBM_AP_INACTIVITY_MIN
#define ATBM_AP_INACTIVITY_MIN	(39)
#endif
```

- [x] **Step 3: Re-run the configuration assertion**

Run the Step 1 PowerShell command again.

Expected: PASS with exit code 0.

### Task 2: Verify integration output

**Files:**
- Build input: `gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h`
- Build output: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`

- [x] **Step 1: Run the existing ATBM WiFi library build**

Use the SDK's established wireless-library build entry point previously used for `libatbm_wifi.a`.

Expected: build exits with code 0 and updates or verifies `libatbm_wifi.a`.

- [x] **Step 2: Verify the exact source scope**

```powershell
Select-String -Path gplib/liba_wifi_src/atbm/atbm6165/include/atbm_config.h -Pattern 'ATBM_AP_INACTIVITY_(MIN|MAX)'
```

Expected: `MIN (39)` and `MAX (1)`.

- [ ] **Step 3: Runtime acceptance check**

Flash the rebuilt firmware and connect the same phone once.

Expected: `[AP] inactivity cfg min=39 max=1`; no immediate `[AP] inactivity sta=...`; DHCP completes without requiring a second association.
