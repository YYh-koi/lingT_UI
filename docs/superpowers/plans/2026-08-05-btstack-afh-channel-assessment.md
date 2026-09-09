# BTstack AFH Channel Assessment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable Controller AFH Channel Assessment with HCI command `0x0C49` during the GPCV2178 BTstack initialization sequence.

**Architecture:** Add the standard command to BTstack's HCI command catalog and add a compile-gated SEND/W4 pair to the existing serial initialization state machine. GPCV2178 enables the gate in its port config, and the completion path prints one concise status line while allowing initialization to continue on non-zero status.

**Tech Stack:** C, BTstack HCI state machine, PowerShell static regression tests, GNU Arm Embedded toolchain.

---

### Task 1: Add a failing AFH initialization regression test

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_btstack_afh_channel_assessment.ps1`
- Test: `btstack/ports/gpcv2178/tests/test_btstack_afh_channel_assessment.ps1`

- [ ] **Step 1: Write the failing test**

The script reads `btstack_config.h`, `hci_cmd.h`, `hci_cmd.c`, `hci.h`, and `hci.c`, then asserts:

```powershell
$checks = @(
    @{ Name = 'GPCV2178 enables AFH assessment'; Pass = $config -match '#define\s+ENABLE_AFH_CHANNEL_ASSESSMENT' },
    @{ Name = 'AFH opcode is 0x0C49'; Pass = $cmdHeader -match 'HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*=\s*HCI_OPCODE\s*\(\s*OGF_CONTROLLER_BASEBAND\s*,\s*0x49\s*\)' },
    @{ Name = 'AFH command takes one byte'; Pass = $cmdSource -match 'hci_write_afh_channel_assessment_mode\s*=\s*\{[\s\S]*?HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*,\s*"1"' },
    @{ Name = 'AFH initialization waits for completion'; Pass = $hciHeader -match 'HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*,\s*\r?\n\s*HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE' },
    @{ Name = 'AFH initialization sends enable once'; Pass = $hciSource -match 'case\s+HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*:[\s\S]*?substate\s*=\s*HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE\s*;[\s\S]*?hci_send_cmd\s*\(\s*&hci_write_afh_channel_assessment_mode\s*,\s*1\s*\)' },
    @{ Name = 'AFH completion prints status'; Pass = $hciSource -match 'btstack: AFH channel assessment status=0x%02X' }
)
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_btstack_afh_channel_assessment.ps1
```

Expected: FAIL with one or more `Missing:` messages because no AFH command/state/config exists yet.

### Task 2: Implement command encoding and serial initialization

**Files:**
- Modify: `btstack/src/hci_cmd.h`
- Modify: `btstack/src/hci_cmd.c`
- Modify: `btstack/src/hci.h`
- Modify: `btstack/src/hci.c`
- Modify: `btstack/ports/gpcv2178/btstack_config.h`
- Test: `btstack/ports/gpcv2178/tests/test_btstack_afh_channel_assessment.ps1`

- [ ] **Step 1: Add the standard opcode and command object**

Add the controller/baseband opcode and single-byte command descriptor:

```c
HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE = HCI_OPCODE (OGF_CONTROLLER_BASEBAND, 0x49),

extern const hci_cmd_t hci_write_afh_channel_assessment_mode;

const hci_cmd_t hci_write_afh_channel_assessment_mode = {
    HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE, "1"
};
```

- [ ] **Step 2: Add compile-gated SEND/W4 states**

After the Inquiry Mode states, add:

```c
#ifdef ENABLE_AFH_CHANNEL_ASSESSMENT
    HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE,
    HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE,
#endif
```

- [ ] **Step 3: Send Enable=1 and wait for Command Complete**

Add the state-machine block before Secure Connections initialization:

```c
#ifdef ENABLE_AFH_CHANNEL_ASSESSMENT
        case HCI_INIT_WRITE_AFH_CHANNEL_ASSESSMENT_MODE:
            if (hci_classic_supported()) {
                hci_stack->substate = HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE;
                hci_send_cmd(&hci_write_afh_channel_assessment_mode, 1);
                break;
            }

            /* fall through */
#endif
```

- [ ] **Step 4: Print the completion status and continue**

Handle both Command Complete and an error Command Status without blocking later initialization:

```c
#ifdef ENABLE_AFH_CHANNEL_ASSESSMENT
        case HCI_INIT_W4_WRITE_AFH_CHANNEL_ASSESSMENT_MODE: {
            uint8_t status = ERROR_CODE_UNSPECIFIED_ERROR;
            if ((hci_event_packet_get_type(packet) == HCI_EVENT_COMMAND_COMPLETE) && (size >= 6u)) {
                status = packet[5];
            } else if ((hci_event_packet_get_type(packet) == HCI_EVENT_COMMAND_STATUS) && (size >= 3u)) {
                status = packet[2];
            }
            BTSTACK_PRINTF("btstack: AFH channel assessment status=0x%02X\r\n", status);
            break;
        }
#endif
```

- [ ] **Step 5: Enable only the GPCV2178 port**

Add next to the Classic/BLE feature flags:

```c
#define ENABLE_AFH_CHANNEL_ASSESSMENT
```

- [ ] **Step 6: Run the AFH test and verify GREEN**

Run the Task 1 command. Expected: PASS with `GPCV2178 initializes AFH channel assessment serially`.

### Task 3: Run regressions and rebuild delivery artifacts

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Verify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`

- [ ] **Step 1: Run focused port regressions**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_btstack_hci_reset_policy.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_btstack_gpcv2178_lifecycle.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\check_btstack_hci_dump_gpcv2178.ps1
```

Expected: all exit code 0; HCI dump remains disabled by configuration.

- [ ] **Step 2: Rebuild `libxstack.a`**

```powershell
$env:Path = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;' + $env:Path
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' -f Xstack_Library_Debug.mk debug
```

Run from `output/wireless_libs/build`. Expected: exit code 0 and a new timestamp on `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`.

- [ ] **Step 3: Inspect the library**

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-nm.exe' '..\..\..\gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a' | Select-String 'hci_write_afh_channel_assessment_mode'
```

Expected: the AFH command symbol is present.

- [ ] **Step 4: Build the main project**

Generate and run the main-project makefile:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\gpcmprj2make.exe' --local -in GPCV2178_turnkey_sdk_release.gpcmprj -cfg Debug -out prjDebug\GPCV2178_turnkey_sdk_release_Debug.mk -windows -tp 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe' -f GPCV2178_turnkey_sdk_release_Debug.mk debug
```

Run the first command from `Project/GPCV2178_Turnkey_SDK` and the second from `Project/GPCV2178_Turnkey_SDK/prjDebug`. Expected: exit code 0, updated `project.elf`, and no unresolved AFH symbols.

- [ ] **Step 5: Report board verification requirement**

After flashing the new `project.elf`, expected boot log:

```text
btstack: AFH channel assessment status=0x00
```

Then repeat WiFi-on A2DP playback and use the existing `btstack disconnect handle/reason` line if the phone disconnects.
