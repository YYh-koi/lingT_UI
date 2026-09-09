# A2DP PCM 512 and Disconnect Reason Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver complete 512-byte decoded SBC PCM frames to the application and log only HCI disconnect handle/reason while full HCI dump remains disabled.

**Architecture:** Increase the existing bounded asynchronous event payload to the maximum SBC PCM frame size. Register a small static HCI event listener in the GPCV2178 port that ignores every event except Disconnection Complete.

**Tech Stack:** C, BTstack HCI event API, PowerShell contract tests, ARM GCC static-library and firmware builds.

---

### Task 1: Add failing PCM and disconnect-diagnostic contracts

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_btstack_pcm_disconnect_diagnostics.ps1`

- [x] **Step 1: Write a focused contract test**

The test reads `btstack_config.h`, `btstack_event_queue.c`, and `btstack_gpcv2178_port.c` and requires:

```powershell
$config -match '#define\s+BTSTACK_EVENT_DATA_MAX\s+512u'
$queue -match 'sample->data_len\s*>\s*\(uint16_t\)BTSTACK_EVENT_DATA_MAX'
$port -match 'HCI_EVENT_DISCONNECTION_COMPLETE'
$port -match 'hci_event_disconnection_complete_get_connection_handle'
$port -match 'hci_event_disconnection_complete_get_reason'
$port -match 'btstack disconnect handle=0x%04X reason=0x%02X'
$config -match '#define\s+BTSTACK_GPCV2178_HCI_DUMP_ENABLE\s+0'
```

It also rejects any call from the new disconnect handler to `hci_dump_init()` or full packet formatting.

- [x] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/ports/gpcv2178/tests/test_btstack_pcm_disconnect_diagnostics.ps1
```

Expected: FAIL because `BTSTACK_EVENT_DATA_MAX` is still 256 and no disconnect-only listener exists.

### Task 2: Accept a full SBC PCM frame

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_config.h:44`

- [x] **Step 1: Increase the bounded event payload**

Change only:

```c
#define BTSTACK_EVENT_DATA_MAX 512u
```

The existing `data_len > BTSTACK_EVENT_DATA_MAX` condition accepts exactly 512 bytes and continues rejecting 513 bytes.

- [x] **Step 2: Run the focused test**

Run the Task 1 test. Expected: it advances past the PCM assertion and fails because the disconnect listener is missing.

### Task 3: Add the independent disconnect reason listener

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_gpcv2178_port.c`

- [x] **Step 1: Add the static callback state and handler**

Add a static `btstack_packet_callback_registration_t`, a registration flag, and this behavior:

```c
if ((packet_type != HCI_EVENT_PACKET) || (packet == NULL) ||
    (size < 6u) ||
    (hci_event_packet_get_type(packet) !=
        HCI_EVENT_DISCONNECTION_COMPLETE)) {
    return;
}
DBG_PRINT("btstack disconnect handle=0x%04X reason=0x%02X\r\n",
    (unsigned int)
        hci_event_disconnection_complete_get_connection_handle(packet),
    (unsigned int)
        hci_event_disconnection_complete_get_reason(packet));
```

- [x] **Step 2: Register once after HCI initialization**

After `hci_init()` and before normal profile operation, set the callback and call `hci_add_event_handler()` only when the registration flag is false.

- [x] **Step 3: Run the focused test and verify GREEN**

Run the Task 1 command. Expected: exit code 0 and a success message confirming PCM 512 and disconnect-only diagnostics.

### Task 4: Rebuild the clean delivery library and firmware

**Files:**
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Generated: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`

- [x] **Step 1: Build the current G+IDE project source set**

The standalone generator could not load the IDE's `armelfgcc` configuration. Use the last complete generated BTstack makefile for its 61-source core, override its output to canonical `bt/libxstack.a`, then compile and archive the 17 newer A2DP/SBC units listed by `BTstack_Library.gpcmprj`. Verify the result has all 78 project units.

- [x] **Step 2: Remove only the generated library object directory and output archive**

Resolve and verify these exact targets remain under the workspace before removal:

```text
Project/WirelessLibraries/BTstack_Library/obj/Debug
gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a
```

This is required because incremental `ar -r` previously retained obsolete namespaced archive members.

- [x] **Step 3: Rebuild and inspect `libxstack.a`**

Build the library with the bundled `make.exe`. Verify the archive contains one `btstack_gpcv2178_port.o` member, the port object does not reference `hci_dump_init`, and it does reference `hci_add_event_handler` plus the disconnect log string.

- [x] **Step 4: Relink the main firmware**

Build with `output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk`. Expected: exit code 0 and a fresh `project.elf`.

- [x] **Step 5: Verify the final ELF**

Require the ELF to contain:

```text
btstack disconnect handle=0x%04X reason=0x%02X
```

and reject:

```text
btstack: hci dump cmd/event enabled
HCI RX EVT len=
HCI %s ACL len=
```

- [x] **Step 6: Run regression contracts**

Run the new focused test plus the existing xStack Demo, AT backend, AT task, HCI reset-policy, and lifecycle contracts. All must exit 0.

- [x] **Step 7: Record source-control status**

Run `git rev-parse --is-inside-work-tree`. This SDK has no Git metadata, so list changed source/test/design/plan files in the handoff rather than committing.
