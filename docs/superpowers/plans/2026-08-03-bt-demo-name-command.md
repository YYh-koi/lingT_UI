# Bluetooth Demo Name Command Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `bt_xstack_demo_start()` submit `AT+NAME=GP-BT-ATBM6165` before it submits `AT+BTEN=1`.

**Architecture:** Keep device-name policy in the application demo. Add one constant and one guarded `bt_xstack_send_at()` call; rely on the existing FIFO AT worker, which completes each queued command before processing the next one.

**Tech Stack:** C, PowerShell contract tests, Generalplus `gpcmprj2make`, GNU Make, ARM GCC 4.9.3.

---

## File Structure

- Modify `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`: assert the exact name command, command order, and failure behavior.
- Modify `Project/wifi_demo/bt_xstack_demo.c`: submit the name command before the enable command.
- Use `output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk`: existing build description containing both demo and audio task sources and the formal `bt/libxstack.a` path.
- Rebuild `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf` and regenerate `project.bin`.

### Task 1: Add the failing demo contract

**Files:**
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Add exact-name and sequencing assertions**

Add these checks after the existing enable-command assertions:

```powershell
if ($source -notmatch '"AT\+NAME=GP-BT-ATBM6165"') {
    throw 'Demo name command is not AT+NAME=GP-BT-ATBM6165'
}
if ($source -notmatch '(?s)status\s*=\s*bt_xstack_send_at\s*\(\s*BT_XSTACK_DEMO_NAME_COMMAND.*?if\s*\(\s*status\s*!=\s*BT_XSTACK_STATUS_OK\s*\)\s*\{\s*bt_audio_task_stop\s*\(\s*\)\s*;\s*return\s+status\s*;\s*\}.*?status\s*=\s*bt_xstack_send_at\s*\(\s*BT_XSTACK_DEMO_ENABLE_COMMAND') {
    throw 'Demo does not complete guarded name submission before Bluetooth enable'
}
```

- [ ] **Step 2: Run the contract test and verify RED**

Run:

```powershell
& 'Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1'
```

Expected: exit code 1 with `Demo name command is not AT+NAME=GP-BT-ATBM6165`.

### Task 2: Submit the name command from the demo

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Define the application name command**

Add before the enable-command definition:

```c
#define BT_XSTACK_DEMO_NAME_COMMAND "AT+NAME=GP-BT-ATBM6165"
#define BT_XSTACK_DEMO_ENABLE_COMMAND "AT+BTEN=1"
```

- [ ] **Step 2: Submit and guard the name command**

Insert immediately after successful PCM callback registration and before the existing enable submission:

```c
    status = bt_xstack_send_at(
        BT_XSTACK_DEMO_NAME_COMMAND,
        (uint16_t)(sizeof(BT_XSTACK_DEMO_NAME_COMMAND) - 1u));
    DBG_PRINT("xstack demo NAME submit status=%d\r\n", status);
    if (status != BT_XSTACK_STATUS_OK) {
        bt_audio_task_stop();
        return status;
    }
```

- [ ] **Step 3: Run the focused contract test and verify GREEN**

Run:

```powershell
& 'Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1'
```

Expected: exit code 0 and `WiFi-gated generic xStack demo contract is present`.

### Task 3: Rebuild and inspect the application image

**Files:**
- Read: `output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk`
- Modify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Modify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Validate the complete Debug makefile**

Run from the workspace root:

```powershell
$makefile = Get-Content -Raw `
  'output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk'
foreach ($required in @(
    'wifi_demo\\bt_xstack_demo.c',
    'wifi_demo\\bt_audio_task.c',
    'atbm6165\\bt\\libxstack.a')) {
    if ($makefile -notmatch [regex]::Escape($required)) {
        throw "Build description is missing $required"
    }
}
```

Expected: exit code 0; both application sources and the formal Bluetooth library path are present.

- [ ] **Step 2: Build the Debug ELF**

Run:

```powershell
$env:PATH = 'D:/IDE/G+IDE_for_ARM_V1.2.0/toolchain/bin;' + $env:PATH
& 'D:/IDE/G+IDE_for_ARM_V1.2.0/toolchain/bin/make.exe' `
  -C 'Project/GPCV2178_Turnkey_SDK' `
  -f (Resolve-Path 'output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk') `
  debug
```

Expected: exit code 0 and a newly linked `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`.

- [ ] **Step 3: Regenerate the flashable binary**

Run:

```powershell
& 'D:/IDE/G+IDE_for_ARM_V1.2.0/toolchain/bin/arm-none-eabi-objcopy.exe' `
  -O binary `
  'Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf' `
  'Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin'
```

Expected: exit code 0 and `project.bin` modification time no earlier than `project.elf`.

- [ ] **Step 4: Verify the final artifacts**

Run:

```powershell
& 'Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1'
& 'D:/IDE/G+IDE_for_ARM_V1.2.0/toolchain/bin/arm-none-eabi-strings.exe' `
  'Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf' |
  Select-String -SimpleMatch 'AT+NAME=GP-BT-ATBM6165'
Get-Item `
  'Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf', `
  'Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin' |
  Select-Object FullName,Length,LastWriteTime
```

Expected: contract exit code 0, the exact name command appears in the ELF, and both artifacts exist with non-zero sizes.

## Version-Control Note

This workspace has no Git metadata, so the implementation cannot be committed here. Preserve all unrelated existing files and generated artifacts.
