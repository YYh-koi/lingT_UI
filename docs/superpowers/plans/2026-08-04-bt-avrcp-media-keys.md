# Bluetooth AVRCP Media Keys Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Map the existing MENU, MODE, and NEXT short-press keys to previous track, play/pause, and next track while AVRCP is connected, preserving all original behavior otherwise.

**Architecture:** Add a Bluetooth-independent media-key callback to the peripheral layer, following the existing call-key callback pattern. The xStack demo tracks `+AVRCPSTAT` readiness and translates generic media actions into the already-supported AVRCP AT commands.

**Tech Stack:** C, FreeRTOS critical sections, BTstack AVRCP AT backend, PowerShell contract tests, Generalplus G+IDE ARM makefiles.

**Repository note:** This SDK directory has no Git metadata. Commit steps are replaced with explicit verification checkpoints; no commit can be created here.

---

### Task 1: Define and enforce the media-key contract

**Files:**
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Write the failing contract assertions**

Add checks for the new generic API, button dispatch, demo registration, AT mappings, and AVRCP state gate:

```powershell
foreach ($contract in @(
        'AP_PERIPHERAL_MEDIA_KEY_PREVIOUS',
        'AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE',
        'AP_PERIPHERAL_MEDIA_KEY_NEXT',
        'ap_peripheral_media_key_handler_t',
        'ap_peripheral_media_key_handler_register')) {
    if ($peripheralHeader -notmatch $contract) {
        throw "Missing generic peripheral media-key API: $contract"
    }
}

$dispatchContracts = @(
    @('ap_peripheral_menu_key_exe', 'AP_PERIPHERAL_MEDIA_KEY_PREVIOUS'),
    @('ap_peripheral_mode_key_exe', 'AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE'),
    @('ap_peripheral_next_key_exe', 'AP_PERIPHERAL_MEDIA_KEY_NEXT')
)
foreach ($dispatch in $dispatchContracts) {
    $pattern = '(?s)void\s+' + $dispatch[0] +
        '\s*\([^)]*\).*?else\s*\{\s*if\s*\(\s*' +
        'ap_peripheral_media_key_dispatch\s*\(\s*' + $dispatch[1] +
        '\s*\)\s*\)\s*\{\s*return\s*;'
    if ($peripheralSource -notmatch $pattern) {
        throw "Missing media dispatch: $($dispatch[0]) -> $($dispatch[1])"
    }
}

foreach ($command in @('AT\+PREV', 'AT\+PLAYPAUSE', 'AT\+NEXT')) {
    if ($source -notmatch $command) {
        throw "Demo is missing media AT command: $command"
    }
}
if (($source -notmatch '"\+AVRCPSTAT=%u%n"') -or
    ($source -notmatch 'value\s*==\s*3u')) {
    throw 'Demo does not parse AVRCP state and detect connected state 3'
}
if ($source -notmatch 'ap_peripheral_media_key_handler_register\s*\(\s*bt_xstack_demo_media_key_handler\s*\)') {
    throw 'Demo does not register its media-key handler'
}
```

- [ ] **Step 2: Run the contract test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: FAIL with `Missing generic peripheral media-key API` because the API has not been added.

### Task 2: Add the Bluetooth-independent peripheral callback

**Files:**
- Modify: `application/task_peripheral_handling/inc/ap_peripheral_handling.h`
- Modify: `application/task_peripheral_handling/src/ap_peripheral_handling.c`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Add the generic media action and handler types**

Add beside the existing call-key types:

```c
typedef enum
{
    AP_PERIPHERAL_MEDIA_KEY_PREVIOUS = 0,
    AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE,
    AP_PERIPHERAL_MEDIA_KEY_NEXT
} ap_peripheral_media_key_action_t;

typedef INT8U (*ap_peripheral_media_key_handler_t)(
    ap_peripheral_media_key_action_t action);
```

Declare:

```c
extern void ap_peripheral_media_key_handler_register(
    ap_peripheral_media_key_handler_t handler);
```

- [ ] **Step 2: Implement registration and dispatch without Bluetooth coupling**

Add a static handler and functions following the call-key pattern:

```c
static ap_peripheral_media_key_handler_t media_key_handler;

void ap_peripheral_media_key_handler_register(
    ap_peripheral_media_key_handler_t handler)
{
    media_key_handler = handler;
}

static INT8U ap_peripheral_media_key_dispatch(
    ap_peripheral_media_key_action_t action)
{
    if (media_key_handler == NULL)
    {
        return 0;
    }
    return media_key_handler(action);
}
```

- [ ] **Step 3: Dispatch only from short-press branches**

Insert these checks as the first statement inside each corresponding `else` branch:

```c
/* ap_peripheral_menu_key_exe */
if (ap_peripheral_media_key_dispatch(
    AP_PERIPHERAL_MEDIA_KEY_PREVIOUS))
{
    return;
}

/* ap_peripheral_mode_key_exe */
if (ap_peripheral_media_key_dispatch(
    AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE))
{
    return;
}

/* ap_peripheral_next_key_exe */
if (ap_peripheral_media_key_dispatch(
    AP_PERIPHERAL_MEDIA_KEY_NEXT))
{
    return;
}
```

Do not change any long-press branches or original fallback statements.

- [ ] **Step 4: Run the contract test**

Run the same PowerShell command. Expected: it still fails, now on the missing demo AVRCP media integration rather than the peripheral API.

### Task 3: Add AVRCP state gating and AT translation in the demo

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Add command constants and shared state**

```c
#define BT_XSTACK_DEMO_PREVIOUS_COMMAND "AT+PREV"
#define BT_XSTACK_DEMO_PLAY_PAUSE_COMMAND "AT+PLAYPAUSE"
#define BT_XSTACK_DEMO_NEXT_COMMAND "AT+NEXT"

static volatile uint8_t bt_xstack_demo_avrcp_connected;
```

- [ ] **Step 2: Parse AVRCP readiness and power-off**

Add a helper that copies one event line, accepts only a complete `+AVRCPSTAT=%u` event, stores connected only for state 3, and clears on exact `+PWRSTAT=0`:

```c
static void bt_xstack_demo_avrcp_state(
    const char *data,
    uint16_t length)
{
    static const char power_off_event[] = "+PWRSTAT=0";
    char line[32];
    uint16_t line_length;
    unsigned int value;
    int consumed = 0;
    uint8_t connected;

    line_length = bt_xstack_demo_copy_event_line(
        line, (uint16_t)sizeof(line), data, length);
    if (line_length == 0u) {
        return;
    }
    if ((line_length == (sizeof(power_off_event) - 1u)) &&
        (memcmp(line, power_off_event,
            sizeof(power_off_event) - 1u) == 0)) {
        connected = 0u;
    } else if ((sscanf(line, "+AVRCPSTAT=%u%n",
                    &value, &consumed) == 1) &&
               (consumed == (int)line_length) &&
               (value >= 1u) && (value <= 3u)) {
        connected = (value == 3u) ? 1u : 0u;
    } else {
        return;
    }

    taskENTER_CRITICAL();
    bt_xstack_demo_avrcp_connected = connected;
    taskEXIT_CRITICAL();
}
```

Call this helper from `bt_xstack_demo_event()` before printing the event.

- [ ] **Step 3: Translate media actions into AT commands**

```c
static INT8U bt_xstack_demo_media_key_handler(
    ap_peripheral_media_key_action_t action)
{
    const char *command;
    uint16_t command_length;
    uint8_t connected;
    int status;

    taskENTER_CRITICAL();
    connected = bt_xstack_demo_avrcp_connected;
    taskEXIT_CRITICAL();
    if (!connected) {
        return 0u;
    }

    switch (action) {
    case AP_PERIPHERAL_MEDIA_KEY_PREVIOUS:
        command = BT_XSTACK_DEMO_PREVIOUS_COMMAND;
        command_length = sizeof(BT_XSTACK_DEMO_PREVIOUS_COMMAND) - 1u;
        break;
    case AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE:
        command = BT_XSTACK_DEMO_PLAY_PAUSE_COMMAND;
        command_length = sizeof(BT_XSTACK_DEMO_PLAY_PAUSE_COMMAND) - 1u;
        break;
    case AP_PERIPHERAL_MEDIA_KEY_NEXT:
        command = BT_XSTACK_DEMO_NEXT_COMMAND;
        command_length = sizeof(BT_XSTACK_DEMO_NEXT_COMMAND) - 1u;
        break;
    default:
        return 0u;
    }

    status = bt_xstack_send_at(command, command_length);
    DBG_PRINT("xstack demo media key action=%u status=%d\r\n",
        (unsigned int)action, status);
    return 1u;
}
```

- [ ] **Step 4: Register the handler after successful xStack initialization**

```c
ap_peripheral_media_key_handler_register(
    bt_xstack_demo_media_key_handler);
```

Place it next to the existing call-key handler registration.

- [ ] **Step 5: Run the focused contract test and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: PASS.

### Task 4: Regression and build verification

**Files:**
- Verify only; no planned source changes.

- [ ] **Step 1: Run the focused demo and lifecycle contracts**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File btstack/ports/gpcv2178/tests/test_btstack_gpcv2178_lifecycle.ps1
```

Expected: both pass.

- [ ] **Step 2: Build the xStack library**

From `Project/WirelessLibraries/BTstack_Library`:

```powershell
$env:PATH='D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;' + $env:PATH
make.exe -B -f ..\..\..\output\wireless_libs\build\Xstack_Library_Debug.mk
```

Expected: `libxstack.a` builds successfully. Existing unrelated compiler warnings may remain.

- [ ] **Step 3: Relink the main project**

From `Project/GPCV2178_Turnkey_SDK`:

```powershell
$env:PATH='D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin;' + $env:PATH
make.exe -f ..\..\output\wireless_libs\build\GPCV2178_A2dpPcm_Debug.mk
```

Expected: `prjDebug/bin/Debug/project.elf` is generated successfully.

- [ ] **Step 4: Record the hardware-verification boundary**

Confirm artifact timestamps and report that the board has not been flashed in this task. Physical MENU/MODE/NEXT behavior requires a subsequent flash and board test.
