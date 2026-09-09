# xStack Library Rename and WiFi-Gated Demo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce `libxstack.a` and start a generic xStack demo by sending `AT+BTEN=1` after WiFi startup succeeds.

**Architecture:** Keep the xStack library independent of WiFi. Add a small application-owned demo module under `Project/wifi_demo`, call it from the successful ATBM WiFi startup branch, and update both G+IDE projects to produce/link `libxstack.a`.

**Tech Stack:** C, FreeRTOS/CMSIS-RTOS, PowerShell contract tests, G+IDE `gpcmprj2make`, ARM GCC.

---

### Task 1: Lock the archive rename contract

**Files:**
- Modify: `btstack/adapter/tests/test_bt_xstack_build_integration.ps1`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Write the failing archive-name assertions**

Require the library project output to end in `/xstack`, require exactly one
active main-project `libxstack.a` link entry, and reject active
`libbtstack.a` link entries.

- [ ] **Step 2: Run the test and verify RED**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/adapter/tests/test_bt_xstack_build_integration.ps1
```

Expected: FAIL because both project files still use `btstack`.

- [ ] **Step 3: Apply the minimal project rename**

Change:

```xml
<Option output="../../../gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/xstack" ... />
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libxstack.a" />
```

Update the adjacent delivery comment to name `libxstack.a`.

- [ ] **Step 4: Run the test and verify GREEN**

Run the same PowerShell command. Expected: PASS.

### Task 2: Specify the application demo contract

**Files:**
- Create: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Create: `Project/Include/bt_xstack_demo.h`
- Create: `Project/wifi_demo/bt_xstack_demo.c`
- Modify: `Project/GPCV2178_Turnkey_SDK/src/main.c`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Write the failing demo contract test**

The test must require:

```text
bt_xstack_demo_start
bt_xstack_init
bt_xstack_send_at
AT+BTEN=1
bt_event_callback_t-compatible byte-span callback
started guard set only after successful command submission
call after atbm_wifi_p2p_mode_enable() == STATUS_OK
demo source/header included once in the main project
```

- [ ] **Step 2: Run the test and verify RED**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: FAIL because the demo files and call site do not exist.

- [ ] **Step 3: Implement the minimal demo**

Public demo header:

```c
#ifndef BT_XSTACK_DEMO_H
#define BT_XSTACK_DEMO_H
int bt_xstack_demo_start(void);
#endif
```

Implementation behavior:

```c
static void bt_xstack_demo_event(
    const char *data, uint16_t length, void *context);

int bt_xstack_demo_start(void)
{
    int status;
    if (bt_xstack_demo_started) return BT_XSTACK_STATUS_OK;
    status = bt_xstack_init(bt_xstack_demo_event, NULL);
    if (status != BT_XSTACK_STATUS_OK) return status;
    status = bt_xstack_send_at(
        "AT+BTEN=1", (uint16_t)(sizeof("AT+BTEN=1") - 1u));
    if (status == BT_XSTACK_STATUS_OK) bt_xstack_demo_started = 1;
    return status;
}
```

The callback prints each byte using the supplied length and does not assume a
terminating NUL.

- [ ] **Step 4: Wire it after WiFi success**

Inside the existing ATBM success block:

```c
if (atbm_wifi_p2p_mode_enable() == STATUS_OK)
{
    INT32S bt_status = bt_xstack_demo_start();
    if (bt_status != 0) {
        ERR_PRINT("xstack demo start failed: %d\r\n", bt_status);
    }
    /* existing one-time socket/RTSP startup */
}
```

Add the demo `.c` and `.h` once to the main G+IDE project.

- [ ] **Step 5: Run the demo contract and verify GREEN**

Run the Task 2 test. Expected: PASS.

### Task 3: Build the renamed library

**Files:**
- Generated: `output/wireless_libs/build/Xstack_Library_Debug.mk`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libxstack.a`

- [ ] **Step 1: Generate the Debug makefile**

Run `gpcmprj2make.exe` from `Project/GPCV2178_Turnkey_SDK` with
`-targets Debug`, the BTstack library `.gpcmprj`, and the G+IDE toolchain path.

- [ ] **Step 2: Build and verify the archive**

Run the bundled `make.exe` from
`Project/WirelessLibraries/BTstack_Library`. Expected: exit 0 and creation of
`libxstack.a`.

- [ ] **Step 3: Verify exported APIs**

```powershell
arm-none-eabi-nm.exe -g --defined-only libxstack.a |
  Select-String 'bt_xstack_(init|send_at)$'
```

Expected: both public symbols are present.

### Task 4: Build and verify the main application

**Files:**
- Generated: `output/wireless_libs/build/GPCV2178_xstack_demo_Debug.mk`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`

- [ ] **Step 1: Generate and build the main Debug project**

Generate with `-targets Debug`, prepend the G+IDE toolchain directory to
`PATH`, and run the bundled `make.exe`. Expected: exit 0.

- [ ] **Step 2: Verify final ELF symbols**

```powershell
arm-none-eabi-nm.exe -g --defined-only project.elf |
  Select-String 'bt_xstack_demo_start|bt_xstack_init|bt_xstack_send_at'
```

Expected: all three symbols are linked into the application.

- [ ] **Step 3: Run Bluetooth regression tests**

Run all PowerShell scripts in Bluetooth and demo `tests` directories.
Expected: zero failures.

- [ ] **Step 4: Remove the obsolete generated archive**

After `libxstack.a` and the main ELF are verified, delete only the exact stale
generated file
`gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libbtstack.a` and confirm that
`libxstack.a` remains.

### Task 5: Keep customer packaging aligned with the archive rename

**Files:**
- Modify: `tools/wireless_lib/tests/test_wireless_lib_tools.ps1`
- Modify: `tools/wireless_lib/WirelessLibTools.psm1`
- Modify: `docs/wireless_static_library_integration.md`
- Modify: `patches/wireless_static_library_integration.patch`

- [ ] **Step 1: Change packaging fixtures and assertions to `libxstack.a`**

Run the packaging contract test and verify that it fails while the tool still
requires `libbtstack.a`.

- [ ] **Step 2: Rename all active packaging paths to `libxstack.a`**

Update archive generation, minimal staging, customer staging, and strip/ranlib
lists. Also update the active delivery guide and integration patch.

- [ ] **Step 3: Run the packaging contract and verify GREEN**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_wireless_lib_tools.ps1
```

Expected: PASS.
