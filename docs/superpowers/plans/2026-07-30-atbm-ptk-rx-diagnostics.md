# ATBM PTK Error Propagation and RX Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make PTK installation failures visible to the WPA state machine and collect enough non-secret RX metadata to identify the remaining decrypt failure.

**Architecture:** Preserve the ATM630-compatible key structures and WSM command flow. Add validation and diagnostics only at the WPA-to-driver, driver-to-firmware, and firmware-to-RX boundaries; leave SDIO and class3 behavior unchanged.

**Tech Stack:** Embedded C, ATBM WSM firmware protocol, Generalplus emIDE ARM GCC library project, PowerShell static regression checks.

---

### Task 1: Establish source-level regression checks

**Files:**
- Test: inline PowerShell checks against `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/wpa_common.c`

- [ ] **Step 1: Run a failing check for PTK return propagation**

Run:

```powershell
$source = Get-Content gplib/liba_wifi_src/atbm/atbm6165/net/wpa/wpa_common.c -Raw
if ($source -notmatch 'ret\s*=\s*atbmwifi_set_key\(priv,0,key_index>>8\)') { throw 'PTK set_key return is still ignored' }
```

Expected: FAIL with `PTK set_key return is still ignored`.

- [ ] **Step 2: Record the existing ATM630 compatibility hashes**

Run `Get-FileHash` for the current and ATM630 `wpa_common.c` and `atbm_key.c`.

Expected: each current/reference pair has the same SHA-256 before modification.

### Task 2: Propagate PTK installation errors

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/wpa_common.c:108`

- [ ] **Step 1: Add a local return value**

Declare `int ret;` in `wpa_common_install_ptk()`.

- [ ] **Step 2: Return the driver result**

Capture `atbmwifi_set_key(priv, 0, key_index >> 8)`, print only cipher/index/link/ret metadata, and return `ret`.

- [ ] **Step 3: Re-run the PTK propagation check**

Expected: PASS.

### Task 3: Validate AP peer lookup and expose WSM add-key result

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/net/atbm_key.c:156`

- [ ] **Step 1: Add a failing source check**

Assert that the AP pairwise path checks `sta_priv == ATBM_NULL` before dereferencing `sta_priv->mac`.

Expected: FAIL on the original code.

- [ ] **Step 2: Add the minimal validation**

When the AP pairwise link has no STA entry, free the reserved key slot and return `-ATBM_EINVAL`.

- [ ] **Step 3: Add non-secret key metadata**

Before and after `wsm_add_key()`, log frame type, link_id, entry index, WSM key type, cipher, peer MAC when applicable, and return code. Never log key bytes.

- [ ] **Step 4: Re-run the validation check**

Expected: PASS.

### Task 4: Enrich RX failure evidence

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_txrx.c:2166`

- [ ] **Step 1: Add a failing source check**

Assert that the RX error path contains one `[RXERR]` format with status, link, frame control, source MAC, flags, and flags2.

Expected: FAIL on the original code.

- [ ] **Step 2: Replace the generic failure print**

Emit one `WIFI_ALWAYS` diagnostic before existing status handling. Retain MIC, no-key, drop, and class3 control flow.

- [ ] **Step 3: Re-run the RX diagnostic check**

Expected: PASS.

### Task 5: Build and verify integration

**Files:**
- Build: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`
- Verify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Build the ATBM source library**

Use the configured emIDE/ARM GCC project. Expected: exit success and refreshed objects for all three modified C files.

- [ ] **Step 2: Verify the archive**

Expected: `libatbm_wifi.a` has a new timestamp and contains the modified objects/symbol strings.

- [ ] **Step 3: Verify the main project link**

Expected: the main project links the refreshed `libatbm_wifi.a`, while `libbtstack.a` remains commented out.

- [ ] **Step 4: Report the hardware test boundary**

State that firmware flashing and a new phone-connect serial log are still required to determine the runtime root cause if WSM returns success.

