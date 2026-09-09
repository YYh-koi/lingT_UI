# GPCV2178 HCI Reset and Packet Summary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Submit one logical HCI Reset during GPCV2178 initialization while preserving H5 reliability, and log complete CMD/EVT packets plus compact ACL/SCO/ISO summaries.

**Architecture:** Keep the generic BTstack Reset retry behavior enabled by default, but disable it from the GPCV2178 configuration and guard only the initial Reset timer. Extend the platform HCI dump backend to dispatch full-packet and summary-packet formatting, and silence the older H5 ACL payload-prefix diagnostics in normal mode.

**Tech Stack:** C, BTstack HCI state machine and dump API, GPCV2178 `DBG_PRINT`, PowerShell contract tests, G+IDE ARM GCC 4.9.3.

---

The workspace is not a Git repository. Commit steps are intentionally omitted; each passing test/build is the recoverable checkpoint.

### Task 1: Disable only the GPCV2178 logical Reset retry

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_btstack_hci_reset_policy.ps1`
- Modify: `btstack/ports/gpcv2178/btstack_config.h`
- Modify: `btstack/src/hci.c:125-127,1899-1910`

- [ ] **Step 1: Write the failing Reset-policy contract test**

Create a PowerShell test that reads `btstack_config.h` and `hci.c`, then requires:

```powershell
$config -match '#define\s+HCI_RESET_RESEND_ENABLE\s+0'
$hci -match '#ifndef\s+HCI_RESET_RESEND_ENABLE\s+#define\s+HCI_RESET_RESEND_ENABLE\s+1\s+#endif'
$initialResetBlock -match '#if\s+HCI_RESET_RESEND_ENABLE'
$initialResetBlock -match 'btstack_run_loop_add_timer\s*\(\s*&hci_stack->timeout\s*\)'
```

The test must isolate the `case HCI_INIT_SEND_RESET:` block and also verify that `hci_send_cmd(&hci_reset)` remains outside the conditional timer guard.

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_btstack_hci_reset_policy.ps1
```

Expected: FAIL because `HCI_RESET_RESEND_ENABLE` does not exist.

- [ ] **Step 3: Add the generic default and platform override**

In `hci.c`, add next to `HCI_RESET_RESEND_TIMEOUT_MS`:

```c
#ifndef HCI_RESET_RESEND_ENABLE
#define HCI_RESET_RESEND_ENABLE 1
#endif
```

Guard only the timer setup in `HCI_INIT_SEND_RESET`:

```c
#if HCI_RESET_RESEND_ENABLE
            btstack_run_loop_set_timer(&hci_stack->timeout, HCI_RESET_RESEND_TIMEOUT_MS);
            btstack_run_loop_set_timer_handler(&hci_stack->timeout, hci_initialization_timeout_handler);
            btstack_run_loop_add_timer(&hci_stack->timeout);
#endif
```

Keep the state transition and command submission unconditional:

```c
            hci_stack->substate = HCI_INIT_W4_SEND_RESET;
            hci_send_cmd(&hci_reset);
```

In `btstack_config.h`, add:

```c
#define HCI_RESET_RESEND_ENABLE 0
```

- [ ] **Step 4: Run the Reset-policy test and verify GREEN**

Run the Step 2 command.

Expected: `GPCV2178 submits one logical HCI Reset while H5 reliability remains enabled`.

### Task 2: Add compact data-packet summaries

**Files:**
- Modify: `btstack/ports/gpcv2178/tests/check_btstack_hci_dump_gpcv2178.ps1`
- Modify: `btstack/ports/gpcv2178/btstack_hci_dump_gpcv2178.c`
- Modify: `btstack/transport/hci_transport_h5.c:568-657`
- Modify: `gpstdlib/print_string/inc/gplib_print_string.h`

- [ ] **Step 1: Extend the dump contract test and verify RED**

Require the dump source to contain these labels and packet cases:

```powershell
'HCI TX CMD len='
'HCI RX EVT len='
'HCI %s ACL len='
'HCI %s SCO len='
'HCI %s ISO len='
'case HCI_ACL_DATA_PACKET:'
'case HCI_SCO_DATA_PACKET:'
'case HCI_ISO_DATA_PACKET:'
```

Require the H5 ACL diagnostic functions to use `H5_DIAG_PRINTF`, not unconditional `BTSTACK_PRINTF`.

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\check_btstack_hci_dump_gpcv2178.ps1
```

Expected: FAIL with the first missing ACL/SCO/ISO summary requirement.

- [ ] **Step 2: Split full-packet and summary formatting**

Implement these focused helpers in `btstack_hci_dump_gpcv2178.c`:

```c
static uint16_t btstack_hci_dump_read_le16(const uint8_t *packet)
{
    return (uint16_t)packet[0] | ((uint16_t)packet[1] << 8);
}

static void btstack_hci_dump_gpcv2178_log_full(const char *label,
                                               const uint8_t *packet,
                                               uint16_t len);
static void btstack_hci_dump_gpcv2178_log_acl(uint8_t in,
                                              const uint8_t *packet,
                                              uint16_t len);
static void btstack_hci_dump_gpcv2178_log_sco(uint8_t in,
                                              const uint8_t *packet,
                                              uint16_t len);
static void btstack_hci_dump_gpcv2178_log_iso(uint8_t in,
                                              const uint8_t *packet,
                                              uint16_t len);
```

`btstack_hci_dump_gpcv2178_log_full` must encode valid CMD/EVT bytes into a bounded hexadecimal text buffer and issue one `DBG_PRINT` call:

```c
DBG_PRINT("%s%u:%s\r\n", label, (unsigned int)len, hex_text);
```

Set `PRINT_BUF_SIZE` to 1024 so the maximum 258-byte HCI command line is not truncated by the platform debug printer.

ACL summaries must safely decode only when the required header bytes exist:

```c
DBG_PRINT("HCI %s ACL len=%u handle=0x%04X pb=%u bc=%u acl_len=%u l2cap_len=%u cid=0x%04X\r\n",
          direction, packet_len, handle, pb, bc, acl_len, l2cap_len, cid);
```

SCO and ISO summaries use equivalent single-line formats with their header flags and data lengths. None of these helpers iterate over or print data payload bytes.

- [ ] **Step 3: Dispatch by HCI packet type**

Use a switch in the registered dump callback:

```c
switch (packet_type) {
case HCI_COMMAND_DATA_PACKET:
    if (in == 0u) {
        btstack_hci_dump_log_full("HCI TX CMD len=", packet, len);
    }
    break;
case HCI_EVENT_PACKET:
    if (in != 0u) {
        btstack_hci_dump_log_full("HCI RX EVT len=", packet, len);
    }
    break;
case HCI_ACL_DATA_PACKET:
    btstack_hci_dump_log_acl(in, packet, len);
    break;
case HCI_SCO_DATA_PACKET:
    btstack_hci_dump_log_sco(in, packet, len);
    break;
case HCI_ISO_DATA_PACKET:
    btstack_hci_dump_log_iso(in, packet, len);
    break;
default:
    break;
}
```

- [ ] **Step 4: Silence legacy H5 payload-prefix output in normal mode**

Replace the two unconditional prints in the H5 ACL diagnostic helpers:

```c
BTSTACK_PRINTF(...)
```

with:

```c
H5_DIAG_PRINTF(...)
```

With `BTSTACK_H5_DIAG_VERBOSE=0`, the unified HCI dump is then the only ACL log source.

- [ ] **Step 5: Run the dump contract and verify GREEN**

Run the Step 1 command.

Expected: `GPCV2178 HCI full command/event dump and data summaries are present`.

### Task 3: Regression and ARM artifact verification

**Files:**
- Rebuild: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Rebuild: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Regenerate: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Run all focused BTstack PowerShell tests**

Run every `.ps1` under:

```text
btstack/adapter/tests
btstack/at/tests
btstack/gap/tests
btstack/profiles/tests
btstack/ports/gpcv2178/tests
```

Expected: all pass with zero failures.

- [ ] **Step 2: Generate and build the BTstack library project**

From `Project/GPCV2178_Turnkey_SDK`, generate the makefile with `--local`, `-targets Debug`, and the bundled toolchain. Run the generated makefile from `Project/WirelessLibraries/BTstack_Library` with the toolchain directory prepended to `PATH`.

Expected: ARM compilation succeeds and `libxstack.a` has a new timestamp.

- [ ] **Step 3: Link the main Debug project**

Generate the main makefile with `-targets Debug` and run it from `Project/GPCV2178_Turnkey_SDK`.

Expected: link exit code zero and a new `project.elf`.

- [ ] **Step 4: Regenerate and inspect the binary**

Run:

```powershell
arm-none-eabi-objcopy.exe -O binary project.elf project.bin
arm-none-eabi-strings.exe project.bin | Select-String 'HCI TX CMD len=|HCI RX EVT len=|HCI %s ACL len=|HCI %s SCO len=|HCI %s ISO len='
```

Expected: all five packet-log formats are found, and `project.bin` is newer than `libxstack.a`.

- [ ] **Step 5: Record hardware acceptance expectations**

After the user burns the image, verify one logical Reset dump line, then its Reset Command Complete, then `Read Local Supported Commands`/the next initialization command. Confirm ACL/SCO/ISO lines contain only metadata and no payload hex dump.
