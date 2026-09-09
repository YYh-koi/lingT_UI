# Bluetooth Name and Reset Retry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Set the default Bluetooth name to `GP-BT-ATBM6165` and restore logical HCI Reset retry with a 1000 ms timeout.

**Architecture:** Keep both policies at their existing boundaries. The adapter property layer owns the Classic/BLE default name, while the GPCV2178 BTstack configuration selects the generic HCI Reset timer behavior and timeout. H5 reliability remains unchanged below HCI.

**Tech Stack:** C, BTstack HCI/H5, PowerShell contract tests, GNU Arm Embedded toolchain.

---

### Task 1: Define the revised contracts

**Files:**
- Modify: `btstack/ports/gpcv2178/tests/test_btstack_hci_reset_policy.ps1`
- Create: `btstack/adapter/tests/test_btstack_default_name.ps1`

- [ ] **Step 1: Replace the single-Reset assertions**

Require the GPCV2178 configuration to contain:

```powershell
$config -match '#define\s+HCI_RESET_RESEND_ENABLE\s+1'
$config -match '#define\s+HCI_RESET_RESEND_TIMEOUT_MS\s+1000'
```

Retain assertions that the initial Reset timer is guarded by
`HCI_RESET_RESEND_ENABLE` and H5 treats command packets as reliable.

- [ ] **Step 2: Add the default-name contract**

Read `btstack/adapter/btstack_property.c` and require:

```powershell
$property -match '#define\s+BTSTACK_PROPERTY_DEFAULT_NAME\s+"GP-BT-ATBM6165"'
$property -notmatch '#define\s+BTSTACK_PROPERTY_DEFAULT_NAME\s+"BTstack Adapter"'
```

Also require `btstack_property_set_local_name` so the AT runtime setter remains available.

- [ ] **Step 3: Run both tests and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/ports/gpcv2178/tests/test_btstack_hci_reset_policy.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/adapter/tests/test_btstack_default_name.ps1
```

Expected: both fail because the port still disables logical Reset retry and the property layer still uses `BTstack Adapter`.

### Task 2: Implement the configuration changes

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_config.h`
- Modify: `btstack/adapter/btstack_property.c`

- [ ] **Step 1: Configure the one-second Reset retry**

Use these GPCV2178 settings:

```c
#define HCI_RESET_RESEND_ENABLE 1
#define HCI_RESET_RESEND_TIMEOUT_MS 1000
```

- [ ] **Step 2: Change the central default name**

Use:

```c
#define BTSTACK_PROPERTY_DEFAULT_NAME "GP-BT-ATBM6165"
```

Do not change the existing Classic/BLE property setters.

- [ ] **Step 3: Run both tests and verify GREEN**

Run the two commands from Task 1. Expected: both pass.

### Task 3: Regression and build verification

**Files:**
- Verify: all `btstack/**/tests/*.ps1`
- Build: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Run all focused BTstack tests**

Execute every PowerShell test below a `btstack/**/tests` directory. Expected: all pass.

- [ ] **Step 2: Rebuild `libxstack.a`**

Run the generated BTstack library makefile from
`Project/WirelessLibraries/BTstack_Library`. Expected: exit code 0.

- [ ] **Step 3: Relink the main project and regenerate BIN**

Run the generated main makefile from `Project/GPCV2178_Turnkey_SDK`, then use
`arm-none-eabi-objcopy -O binary` on the new ELF. Expected: both commands exit 0.

- [ ] **Step 4: Inspect final artifacts**

Use `arm-none-eabi-strings` to require `GP-BT-ATBM6165` in both the archive and
ELF and reject `BTstack Adapter`. Verify the library timestamp precedes or
matches the final ELF timestamp and the BIN is regenerated after linking.

No commit step is included because this workspace is not a Git repository.
