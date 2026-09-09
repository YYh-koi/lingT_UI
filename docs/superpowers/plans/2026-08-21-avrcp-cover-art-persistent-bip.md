# AVRCP Cover Art Persistent BIP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep one BIP/OBEX session for the AVRCP lifetime and automatically fetch only the latest changed Image Handle without canceling on track changes.

**Architecture:** `btstack_avrcp_cover_art.c` remains the policy owner. It keeps the active native GET alive, marks its result stale after a track/handle change, suppresses stale payload delivery, and coalesces subsequent handles into one latest pending request. `btstack_bip_client.c` already returns to `CONNECTED` after successful GET, so no protocol-layer change is required.

**Tech Stack:** C99, BTstack AVRCP Cover Art/BIP client, existing PowerShell host tests, ARM GCC and GPCV2178 build.

---

### Task 1: Lock Persistent-Session Policy with Failing Tests

**Files:**
- Modify: `btstack/profiles/tests/test_btstack_avrcp_cover_art.c`
- Test: `btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1`

- [ ] **Step 1: Replace reconnect expectations with persistent-session expectations**

Add assertions equivalent to:

```c
btstack_avrcp_cover_art_track_changed();
btstack_avrcp_cover_art_handle_received((const uint8_t *) "2222222", 7u);
CHECK(cancel_calls == 0u);
emit_data(old_generation);
emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, old_generation);
CHECK(connect_calls == 1u);
CHECK(set_object_calls == 2u);
CHECK(strcmp(last_handle, "2222222") == 0);
```

Also assert stale DATA/COMPLETED are not forwarded, three rapid handle changes start only the last pending handle, and an unchanged handle does not start another GET.

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File btstack/profiles/tests/test_btstack_avrcp_cover_art.ps1
```

Expected: failure because the current policy calls `btstack_bip_client_cancel()` and reconnects after track change.

### Task 2: Implement Latest-Handle Persistent BIP Scheduling

**Files:**
- Modify: `btstack/profiles/btstack_avrcp_cover_art.c`
- Test: `btstack/profiles/tests/test_btstack_avrcp_cover_art.c`

- [ ] **Step 1: Add active-operation staleness state**

Add one byte to the private context:

```c
uint8_t active_stale;
```

Set it when a track changes or a different handle arrives during an active operation. Clear it when a new operation starts.

- [ ] **Step 2: Stop canceling on track and handle changes**

Change track/handle policy so that active native GET remains running, while `pending_handle` and `pending_generation` are latest-wins. Preserve `obex_ready` and the current handle across track changes so identical handles remain deduplicated.

- [ ] **Step 3: Consume stale terminal events internally**

For the active stale generation, do not forward DATA, PROPERTIES, or successful terminal events to the application callback. Still update internal terminal state and call `btstack_avrcp_cover_art_start_pending()` so the latest pending handle starts on the same BIP connection.

For a true BIP error, retain automatic reconnect while AVRCP remains connected; connection-ready starts the pending handle if present.

- [ ] **Step 4: Run focused test and verify GREEN**

Run the Task 1 command. Expected: `PASS: AVRCP Cover Art automatic policy` and process exit code 0.

### Task 3: Regress, Build and Package Firmware

**Files:**
- Rebuild: `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`
- Rebuild: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Generate: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Run focused profile and application tests**

Run fresh processes for cover-art policy, BIP client, AVRCP profile split, upstream Cover Art contract, Bluetooth-music receiver, and Bluetooth-music integration. Expected: all exit 0.

- [ ] **Step 2: Build the API library**

```powershell
Set-Location Project/WirelessLibraries/BTstack_API_Library
./build_btstack_api.ps1 -Jobs 4
```

Expected: exit 0; only existing upstream AVDTP/AVRCP warnings are allowed.

- [ ] **Step 3: Relink firmware and generate binary**

```powershell
$env:PATH='D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;D:\mingw32\bin;' + $env:PATH
Set-Location Project/GPCV2178_Turnkey_SDK
mingw32-make.exe -f GPCV2178_turnkey_sdk_release_Debug.mk -j4
arm-none-eabi-objcopy.exe -O binary prjDebug/bin/Debug/project.elf prjDebug/bin/Debug/project.bin
```

Expected: exit 0; `project.bin` is newer than `libxstack_api.a` and SHA256 is recorded for handoff.
