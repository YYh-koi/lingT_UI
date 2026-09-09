# BTstack Feature Configuration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the duplicated configuration headers and generic `BTSTACK_ENABLE_*` switches with one canonical family/Profile configuration plus one clearly named GPCV2178 platform configuration.

**Architecture:** `btstack/include/btstack_config.h` is the only file with that name and owns stack-family, Profile, bonding, and platform-neutral capacity choices. `btstack/ports/gpcv2178/btstack_gpcv2178_config.h` includes it and adds only platform/RTOS/transport settings. Profile code uses direct parent-and-child expressions; disabled APIs retain unsupported/default ABI stubs.

**Tech Stack:** C99 preprocessor configuration, BTstack Classic/LE stack, PowerShell contracts, G+ ARM GCC project generator/build.

---

### Task 1: Establish the canonical configuration contract

**Files:**
- Create: `btstack/core/tests/test_btstack_feature_config.ps1`
- Modify: `btstack/core/tests/test_btstack_platform.ps1`

- [ ] **Step 1: Write a failing structural and compile-matrix test**

The test must enumerate `btstack/**/btstack_config.h`, require exactly `btstack/include/btstack_config.h`, require `btstack/ports/gpcv2178/btstack_gpcv2178_config.h`, and reject production occurrences of `BTSTACK_ENABLE_CORE`, `BTSTACK_ENABLE_CLASSIC`, `BTSTACK_ENABLE_BLE`, `BTSTACK_ENABLE_A2DP_SINK`, `BTSTACK_ENABLE_HFP_HF`, `BTSTACK_ENABLE_PBAP_CLIENT`, `BTSTACK_ENABLE_HID_DEVICE`, `BTSTACK_ENABLE_HOGP_DEVICE`, `BTSTACK_ENABLE_GATT`, and `BTSTACK_ENABLE_BONDING`.

Compile small C99 fixtures with command-line macro overrides and assert these expressions:

```c
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
int selected_a2dp;
#endif
#if BTSTACK_LE && BTSTACK_GATT_SERVER
int selected_gatt_server;
#endif
```

Cover Classic off, LE off, AVRCP CT-only, TG-only, both, GATT Server-only, and default GPCV2178 selection.

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_feature_config.ps1
```

Expected: FAIL because two `btstack_config.h` files exist and old `BTSTACK_ENABLE_*` macros remain.

### Task 2: Consolidate configuration files

**Files:**
- Create: `btstack/include/btstack_config.h`
- Create: `btstack/ports/gpcv2178/btstack_gpcv2178_config.h`
- Delete: `btstack/include/adapter/btstack_config.h`
- Delete: `btstack/ports/gpcv2178/btstack_config.h`
- Modify: `btstack/include/adapter/btstack_types.h`
- Modify: all `.gpcmprj`, generator, generated `.mk`, test compile commands, and stubs that force-include the old port path.

- [ ] **Step 1: Create the canonical feature table**

Define overrideable `0/1` values:

```c
#define BTSTACK_CORE 1
#define BTSTACK_CLASSIC 1
#define BTSTACK_LE 1
#define BTSTACK_A2DP_SINK 1
#define BTSTACK_HFP_HF 1
#define BTSTACK_AVRCP_CONTROLLER 1
#define BTSTACK_AVRCP_TARGET 1
#define BTSTACK_PBAP_CLIENT 1
#define BTSTACK_HID_DEVICE 0
#define BTSTACK_GATT_CLIENT 0
#define BTSTACK_GATT_SERVER 0
#define BTSTACK_HOGP_DEVICE 0
#define BTSTACK_BONDING 0
```

Retain platform-neutral capacities. Derive upstream presence macros only from the family switches:

```c
#if BTSTACK_CLASSIC
#define ENABLE_CLASSIC
#endif
#if BTSTACK_LE
#define ENABLE_BLE
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_PERIPHERAL
#endif
```

- [ ] **Step 2: Rename the GPCV2178 configuration**

Move target-only includes and settings into `btstack_gpcv2178_config.h`, start it with:

```c
#ifndef BTSTACK_GPCV2178_CONFIG_H
#define BTSTACK_GPCV2178_CONFIG_H
#include "btstack_config.h"
```

Do not redefine protocol/Profile switches in the port file.

- [ ] **Step 3: Update include and force-include paths**

Change public types to:

```c
#include "btstack_config.h"
```

Change GPCV2178 project options from:

```text
-include ../../../btstack/ports/gpcv2178/btstack_config.h
```

to:

```text
-include ../../../btstack/ports/gpcv2178/btstack_gpcv2178_config.h
```

- [ ] **Step 4: Run the feature-config test until the file/layout section is GREEN**

Run the Task 1 command. Expected: file count and old-path checks PASS; Profile selection checks may remain RED until Task 3.

### Task 3: Migrate family and Profile guards

**Files:**
- Modify: `btstack/core/btstack_adapter.c`
- Modify: `btstack/core/btstack_property.c`
- Modify: `btstack/core/btstack_profile_manager.c`
- Modify: `btstack/profiles/btstack_a2dp_sink.c`
- Modify: `btstack/profiles/btstack_a2dp_media.c`
- Modify: `btstack/profiles/btstack_hfp_hf.c`
- Modify: `btstack/profiles/btstack_avrcp.c`
- Modify: `btstack/profiles/btstack_avrcp_controller.c`
- Modify: `btstack/profiles/btstack_avrcp_target.c`
- Modify: `btstack/profiles/btstack_pbap_client.c`
- Modify: `btstack/profiles/btstack_hid_device.c`
- Modify: `btstack/profiles/btstack_hogp_device.c`
- Modify: `btstack/profiles/btstack_gatt.c`
- Modify: relevant focused contracts and fixtures.

- [ ] **Step 1: Replace core/family guards**

Use `BTSTACK_CLASSIC`, `BTSTACK_LE`, and `BTSTACK_BONDING` in adapter/property code. Generic advertising, scanning, Central, Peripheral, and privacy behavior is guarded only by `BTSTACK_LE`.

- [ ] **Step 2: Replace Classic Profile guards**

Use direct expressions:

```c
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
#if BTSTACK_CLASSIC && BTSTACK_PBAP_CLIENT
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
```

AVRCP common session:

```c
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
```

AVRCP roles:

```c
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_TARGET
```

Session setup calls only enabled roles; disabled role internal functions return unsupported or no-op without referencing upstream role symbols.

- [ ] **Step 3: Replace LE Profile guards**

Use:

```c
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
#if BTSTACK_LE && BTSTACK_GATT_SERVER
```

The current GATT implementation remains Server-only. `BTSTACK_GATT_CLIENT` has no implementation references.

- [ ] **Step 4: Condition Profile Manager ownership and linkage**

Only initialize enabled Profiles. Compile A2DP-to-AVRCP linkage only when A2DP and at least one AVRCP role are active. Compile HFP-to-PBAP linkage only when both child Profiles are active. Preserve public state and disabled return semantics.

- [ ] **Step 5: Run macro matrix and focused Profile tests**

Run:

```powershell
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_feature_config.ps1
powershell.exe -NoProfile -File btstack/core/tests/test_btstack_profile_manager.ps1
powershell.exe -NoProfile -File btstack/profiles/tests/check_btstack_avrcp_profile.ps1
```

Expected: all PASS.

### Task 4: Regenerate and verify selected builds

**Files:**
- Modify generated project makefiles through official generators.
- Modify packaging/layout contracts that intentionally reference the renamed platform configuration.

- [ ] **Step 1: Regenerate API, AT, and Legacy project makefiles**

Run the official API and AT build scripts and the wireless project generator plus Legacy G+ generator. Confirm every generated compile rule force-includes `btstack_gpcv2178_config.h` and contains no old path.

- [ ] **Step 2: Build all three archives**

Run official API and AT `-B` builds and Legacy `mingw32-make -B -j4`. Expected: exit 0; only pre-existing upstream warnings may remain.

- [ ] **Step 3: Run focused verification**

Run all Profile scripts plus platform, selected-build variants/artifacts, unified-layout, frontend-separation, wireless-project, API integration, AT worker/event, and package contracts.

- [ ] **Step 4: Audit output**

Use ARM `ar`/`nm` to confirm each archive has exactly one canonical configuration selection, expected Profile objects, one AVRCP common/CT/TG object, and no duplicate public Profile symbols. Verify no production or generated file contains project-owned `BTSTACK_ENABLE_*` or the removed config paths.

No commit steps are included because this shared workspace is not managed as a Git working tree.
