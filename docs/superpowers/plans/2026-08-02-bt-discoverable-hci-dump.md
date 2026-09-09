# Bluetooth Discoverability and HCI Dump Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the powered Classic Bluetooth device discoverable/connectable by default and print HCI Command/Event packets as UART1 hexadecimal diagnostics.

**Architecture:** Keep startup policy in the AT backend so the adapter GAP cache and controller requests stay consistent. Add a focused GPCV2178 HCI dump backend that filters out ACL/SCO/ISO traffic, register it before `hci_init()`, and gate it with a platform compile-time switch.

**Tech Stack:** C, BTstack HCI dump API, GPCV2178 `DBG_PRINT`, PowerShell contract tests, G+IDE `gpcmprj2make`, ARM GCC.

---

### Task 1: Default discoverable and connectable policy

**Files:**
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`
- Modify: `btstack/at/btstack_at_backend.c:886-905`

- [ ] **Step 1: Write the failing contract assertions**

Add assertions requiring `btstack_at_backend_init()` to call both adapter APIs:

```powershell
$checks += @{
    Name = 'default connectable policy'
    Pass = $source -match 'btstack_gap_set_connectable\s*\(\s*true\s*\)'
}
$checks += @{
    Name = 'default discoverable policy'
    Pass = $source -match 'btstack_gap_set_discoverable\s*\(\s*true\s*\)'
}
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
& 'btstack\at\tests\test_btstack_at_backend_contract.ps1'
```

Expected: FAIL reporting the missing default connectable/discoverable policy.

- [ ] **Step 3: Implement the cached startup policy**

In `btstack_at_backend_init()`, after SSP setup and before the final state synchronization, add checked calls:

```c
status = btstack_gap_set_connectable(true);
if (status != BTSTACK_STATUS_OK) {
    return status;
}
status = btstack_gap_set_discoverable(true);
if (status != BTSTACK_STATUS_OK) {
    return status;
}
btstack_at_backend_sync_gap_state();
```

- [ ] **Step 4: Run the test and verify GREEN**

Run the same PowerShell test. Expected: PASS with `Generic AT General/GAP/Profile backend contract is present`.

### Task 2: GPCV2178 Command/Event HCI dump backend

**Files:**
- Create: `btstack/ports/gpcv2178/btstack_hci_dump_gpcv2178.h`
- Create: `btstack/ports/gpcv2178/btstack_hci_dump_gpcv2178.c`
- Create: `btstack/ports/gpcv2178/tests/check_btstack_hci_dump_gpcv2178.ps1`
- Modify: `btstack/ports/gpcv2178/btstack_config.h`
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`

- [ ] **Step 1: Write the failing dump contract**

The PowerShell test shall require:

```powershell
$required = @(
    'BTSTACK_GPCV2178_HCI_DUMP_ENABLE',
    'btstack_hci_dump_gpcv2178_instance',
    'hci_dump_init',
    'HCI_COMMAND_DATA_PACKET',
    'HCI_EVENT_PACKET',
    'HCI TX CMD len=',
    'HCI RX EVT len='
)
```

It shall also verify that the new `.c` file is present in `BTstack_Library.gpcmprj` and that ACL/SCO/ISO packet types are not selected for output.

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
& 'btstack\ports\gpcv2178\tests\check_btstack_hci_dump_gpcv2178.ps1'
```

Expected: FAIL because the backend and registration do not exist.

- [ ] **Step 3: Add the dump interface and implementation**

Expose:

```c
const hci_dump_t *btstack_hci_dump_gpcv2178_instance(void);
```

Implement an `hci_dump_t` backend whose packet callback accepts only outgoing `HCI_COMMAND_DATA_PACKET` and incoming `HCI_EVENT_PACKET`, prints `HCI TX CMD` or `HCI RX EVT`, prints `len`, then prints every byte as two-digit uppercase hexadecimal. Its message callback is a no-op because this mode is packet-only.

- [ ] **Step 4: Register the backend behind an enabled macro**

Add to `btstack_config.h`:

```c
#define BTSTACK_GPCV2178_HCI_DUMP_ENABLE 1
```

In `btstack_gpcv2178_port.c`, before `hci_init()`:

```c
#if BTSTACK_GPCV2178_HCI_DUMP_ENABLE
    hci_dump_init(btstack_hci_dump_gpcv2178_instance());
    hci_dump_enable_packet_log(true);
#endif
```

Add the new source file beside the other GPCV2178 port units in `BTstack_Library.gpcmprj`.

- [ ] **Step 5: Run the dump contract and verify GREEN**

Run the Task 2 test. Expected: PASS and a message confirming Command/Event-only HCI dump wiring.

### Task 3: Regression tests and builds

**Files:**
- Generated: `output/wireless_libs/build/BTstack_HciDump_Debug.mk`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Generated: `output/wireless_libs/build/GPCV2178_HciDump_Debug.mk`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Run all focused BTstack contract tests**

Run every `.ps1` under `btstack/adapter/tests`, `btstack/at/tests`, `btstack/gap/tests`, `btstack/profiles/tests`, and `btstack/ports/gpcv2178/tests`. Expected: every script exits zero.

- [ ] **Step 2: Generate and build `libxstack.a`**

Run:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$makefile = 'output\wireless_libs\build\BTstack_HciDump_Debug.mk'
New-Item -ItemType Directory -Force -Path (Split-Path $makefile) | Out-Null
& "$toolDir\gpcmprj2make.exe" --local -in 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj' -cfg Debug -out $makefile -tp $toolDir -windows
& "$toolDir\make.exe" -f $makefile
```

Expected: exit 0 and a new `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`.

- [ ] **Step 3: Verify archive symbols and freshness**

Use `arm-none-eabi-nm.exe` to require `btstack_hci_dump_gpcv2178_instance`, `bt_xstack_init`, and `bt_xstack_send_at`. Require the archive timestamp to be newer than every modified BTstack `.c/.h` file.

- [ ] **Step 4: Generate and build the main Debug firmware**

Run the same converter and make tools against `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`, outputting `GPCV2178_HciDump_Debug.mk`. Expected: exit 0 and a new `project.bin` newer than `libxstack.a`.

- [ ] **Step 5: Record hardware acceptance checks**

After burning the new `project.bin`, require:

```text
HCI TX CMD len=3: 03 0C 00
HCI RX EVT ...
xstack event: +PWRSTAT=1
xstack event: +DEVSTAT=3
```

Then confirm a phone discovers `bluetooth_auto` and can initiate a connection.

This workspace has no Git metadata, so per-task commits are not possible; preserve the exact modified-file list and fresh test/build output instead.
