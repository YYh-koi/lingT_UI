# Bluetooth Call Key Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make physical KEY1 answer an incoming HFP call and KEY2 reject or hang up a call without changing their existing no-call or long-press behavior.

**Architecture:** The peripheral key module exposes a vendor-neutral optional call-key callback. `bt_xstack_demo` registers the callback, derives call state from length-bounded AT events, and submits `AT+HFPANSW` or `AT+HFPCHUP` only in applicable states.

**Tech Stack:** C, Generalplus application key handling, xStack AT API, PowerShell contract tests, GPCV2178 make-based firmware build.

---

### Task 1: Add failing call-key contract checks

**Files:**
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [x] **Step 1: Load the peripheral header and source in the contract test**

Add paths, existence checks, and raw file reads for:

```powershell
$peripheralHeaderPath = Join-Path $root 'application/task_peripheral_handling/inc/ap_peripheral_handling.h'
$peripheralSourcePath = Join-Path $root 'application/task_peripheral_handling/src/ap_peripheral_handling.c'
$peripheralHeader = Get-Content -Raw -LiteralPath $peripheralHeaderPath
$peripheralSource = Get-Content -Raw -LiteralPath $peripheralSourcePath
```

- [x] **Step 2: Add precise API, key-routing, event, and command checks**

Require the generic action enum and registration API; require OK short press to dispatch `AP_PERIPHERAL_CALL_KEY_ANSWER`, PREV short press to dispatch `AP_PERIPHERAL_CALL_KEY_HANGUP`; require the Demo to parse `+HFPRING=`, `+HFPSTAT=`, and `+PWRSTAT=0`; require exact command constants `AT+HFPANSW` and `AT+HFPCHUP`; require Demo startup to register its callback.

- [x] **Step 3: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: FAIL with the first missing call-key API or routing contract, because production code does not yet provide the feature.

### Task 2: Implement the vendor-neutral peripheral call-key hook

**Files:**
- Modify: `application/task_peripheral_handling/inc/ap_peripheral_handling.h`
- Modify: `application/task_peripheral_handling/src/ap_peripheral_handling.c`

- [x] **Step 1: Add the public callback types and registration declaration**

Add to the peripheral header:

```c
typedef enum {
	AP_PERIPHERAL_CALL_KEY_ANSWER = 0,
	AP_PERIPHERAL_CALL_KEY_HANGUP
} ap_peripheral_call_key_action_t;

typedef INT8U (*ap_peripheral_call_key_handler_t)(
	ap_peripheral_call_key_action_t action);

extern void ap_peripheral_call_key_handler_register(
	ap_peripheral_call_key_handler_t handler);
```

- [x] **Step 2: Store, register, and dispatch the optional callback**

Add a static callback pointer, a registration function, and an internal dispatcher that returns `0` when no callback is registered:

```c
static ap_peripheral_call_key_handler_t call_key_handler;

void ap_peripheral_call_key_handler_register(
	ap_peripheral_call_key_handler_t handler)
{
	call_key_handler = handler;
}

static INT8U ap_peripheral_call_key_dispatch(
	ap_peripheral_call_key_action_t action)
{
	if (call_key_handler == NULL) {
		return 0;
	}
	return call_key_handler(action);
}
```

- [x] **Step 3: Intercept only the short-press paths**

At the beginning of the existing `else` short-press branch in `ap_peripheral_ok_key_exe()`, return when the ANSWER action is consumed. Do the same with HANGUP in `ap_peripheral_prev_key_exe()`. Leave both long-press branches unchanged.

- [x] **Step 4: Run the contract test**

Run the Task 1 command. Expected: it advances past the peripheral API/routing assertions and fails on the missing Demo HFP state handling.

### Task 3: Implement HFP event state and AT command handling in the Demo

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`

- [x] **Step 1: Add exact commands and local HFP state constants**

Add:

```c
#define BT_XSTACK_DEMO_ANSWER_COMMAND "AT+HFPANSW"
#define BT_XSTACK_DEMO_HANGUP_COMMAND "AT+HFPCHUP"

enum {
	BT_XSTACK_DEMO_HFP_IDLE = 1,
	BT_XSTACK_DEMO_HFP_CONNECTING,
	BT_XSTACK_DEMO_HFP_CONNECTED,
	BT_XSTACK_DEMO_HFP_OUTGOING,
	BT_XSTACK_DEMO_HFP_INCOMING,
	BT_XSTACK_DEMO_HFP_ACTIVE,
	BT_XSTACK_DEMO_HFP_HELD,
	BT_XSTACK_DEMO_HFP_ACTIVE_WITH_INCOMING,
	BT_XSTACK_DEMO_HFP_ACTIVE_WITH_HELD,
	BT_XSTACK_DEMO_HFP_MULTIPARTY
};
```

Store `volatile uint8_t` ringing and HFP state values.

- [x] **Step 2: Parse call events without assuming NUL termination**

Add a bounded helper that copies at most a small local buffer, terminates it locally, and uses exact-prefix `memcmp()` plus `sscanf()` to update:

- ringing from `+HFPRING=0/1`;
- HFP state from `+HFPSTAT=<state>`;
- both values reset on `+PWRSTAT=0`.

Call the helper from `bt_xstack_demo_event()` beside the existing PCM configuration parser.

- [x] **Step 3: Implement the registered call-key handler**

For ANSWER, consume only while ringing or in INCOMING / ACTIVE_WITH_INCOMING and submit `BT_XSTACK_DEMO_ANSWER_COMMAND`. For HANGUP, consume while ringing or in OUTGOING, INCOMING, ACTIVE, HELD, ACTIVE_WITH_INCOMING, ACTIVE_WITH_HELD, or MULTIPARTY and submit `BT_XSTACK_DEMO_HANGUP_COMMAND`. Return `1` after applicable-state submission even when submission fails; log the action and status. Return `0` in unrelated states.

- [x] **Step 4: Register the handler during successful Demo startup**

After `bt_xstack_init()` succeeds, call:

```c
ap_peripheral_call_key_handler_register(
	bt_xstack_demo_call_key_handler);
```

The handler remains safe before HFP setup because its initial state is IDLE/no ring and returns not consumed.

- [x] **Step 5: Run the contract test and verify GREEN**

Run the Task 1 command. Expected final line:

```text
WiFi-gated generic xStack demo contract is present
```

with process exit code 0.

### Task 4: Regression and firmware build verification

**Files:**
- Verify: all files changed in Tasks 1-3
- Build output: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`

- [x] **Step 1: Run focused Bluetooth tests**

Run the Demo contract test plus the existing AT parser/backend contract tests discovered under `btstack/at/tests`. Expected: all commands exit 0.

- [x] **Step 2: Build the main firmware**

Run the established application build using:

```powershell
make -f output/wireless_libs/build/GPCV2178_A2dpPcm_Debug.mk
```

Expected: exit code 0 and a freshly written `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`.

- [x] **Step 3: Inspect the final change set and requirements**

Confirm exact command strings, short-press-only dispatch, unchanged long-press bodies, no direct Bluetooth include/dependency in the peripheral module, bounded event parsing, and no unrelated file edits.

- [x] **Step 4: Record source-control status**

Run `git rev-parse --is-inside-work-tree`. This SDK directory currently has no Git metadata, so record the changed files in the handoff instead of creating a commit.
