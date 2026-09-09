# ATBM FreeRTOS Thread Shutdown Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prevent `atbm_wifi_stop_p2p()` from returning out of a FreeRTOS task entry while retaining a timeout-based forced-delete fallback.

**Architecture:** The running task owns normal self-deletion: the common wrapper logs completion, caches its handle, signals `stopEv`, and calls `atbm_ThreadStopEvent()`. The stopping task waits for that signal and only calls `atbm_stopThread(handle)` when the wait times out, preventing both task-entry return and double deletion.

**Tech Stack:** C99, FreeRTOS tasks/semaphores, PowerShell source-contract tests.

---

### Task 1: Add a failing thread-shutdown contract

**Files:**
- Create: `tools/wireless_lib/tests/test_atbm_thread_shutdown.ps1`
- Test: `tools/wireless_lib/tests/test_atbm_thread_shutdown.ps1`

- [x] **Step 1: Create the focused source-contract test**

Create the test with this complete content:

```powershell
$ErrorActionPreference = 'Stop'
$script:Failures = 0

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

function Assert-NotMatch {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -match $Pattern) {
        $script:Failures++
        Write-Host "FAIL: $Message" -ForegroundColor Red
    }
}

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspace = (Resolve-Path (Join-Path $testDirectory '..\..\..')).Path
$threadPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\hal\atbm_thread.c'
$osThreadPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\os\freertos\atbm_os_thread.c'
$p2pPath = Join-Path $workspace `
    'gplib\liba_wifi_src\atbm\atbm6165\net\wpa\p2p\p2p_main.c'
$portPath = Join-Path $workspace `
    'os\freertos_v202012\Source\portable\GCC\ARM_CA9\port.c'

foreach ($path in @($threadPath, $osThreadPath, $p2pPath, $portPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing thread shutdown source: $path"
    }
}

$threadText = Get-Content -LiteralPath $threadPath -Raw
$osThreadText = Get-Content -LiteralPath $osThreadPath -Raw
$p2pText = Get-Content -LiteralPath $p2pPath -Raw
$portText = Get-Content -LiteralPath $portPath -Raw

Assert-Match `
    -Text $threadText `
    -Pattern 'pAtbm_thread_t\s+thread_id[\s\S]{0,1800}thread_id\s*=\s*thread->thread' `
    -Message 'thread wrapper caches the task handle before publishing completion'
Assert-Match `
    -Text $threadText `
    -Pattern 'wifi_printk\s*\([^;]*Thread %s stop[^;]*;[\s\S]{0,180}atbm_os_wakeup_event\s*\(\s*&thread->stopEv\s*\)\s*;[\s\S]{0,120}atbm_ThreadStopEvent\s*\(\s*thread_id\s*\)' `
    -Message 'thread wrapper logs, signals completion, then self-deletes'
Assert-NotMatch `
    -Text $threadText `
    -Pattern 'atbm_os_wakeup_event\s*\(\s*&thread->stopEv\s*\)\s*;[\s\S]{0,160}\bthread->' `
    -Message 'thread wrapper does not dereference its wrapper after publishing completion'

Assert-Match `
    -Text $threadText `
    -Pattern 'ret\s*=\s*atbm_os_wait_event_timeout\s*\(\s*&thread->stopEv\s*,\s*10\s*\*\s*ATBM_HZ\s*\)' `
    -Message 'stopper records whether cooperative shutdown completed'
Assert-Match `
    -Text $threadText `
    -Pattern 'if\s*\(\s*ret\s*==\s*0\s*\)[\s\S]{0,120}atbm_stopThread\s*\(\s*thread->thread\s*\)' `
    -Message 'stopper force-deletes only after timeout'
Assert-Match `
    -Text $threadText `
    -Pattern 'return\s*\(\s*ret\s*==\s*0\s*\)\s*\?\s*-1\s*:\s*0\s*;' `
    -Message 'stopper returns deterministic success or timeout failure'

Assert-Match `
    -Text $osThreadText `
    -Pattern 'freertos_ThreadStopEvent[\s\S]{0,180}THREAD_RETURN\s*\(\s*0\s*\)' `
    -Message 'FreeRTOS normal thread exit self-deletes the current task'
Assert-Match `
    -Text $p2pText `
    -Pattern 'p2p_main_task_release[\s\S]{0,700}msg\.event\s*=\s*NOTHING[\s\S]{0,600}atbm_stopThreadInternal' `
    -Message 'P2P release still requests cooperative task shutdown'
Assert-Match `
    -Text $portText `
    -Pattern 'static\s+void\s+prvTaskExitError\s*\(\s*void\s*\)[\s\S]{0,300}task must not exit or attempt to return[\s\S]{0,500}portDISABLE_INTERRUPTS' `
    -Message 'test remains tied to the ARM FreeRTOS task-return trap'

if ($script:Failures -ne 0) {
    throw "$script:Failures test assertion(s) failed"
}

Write-Host 'PASS: ATBM FreeRTOS thread shutdown contract'
```

- [x] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_thread_shutdown.ps1
```

Expected: FAIL because `atbm_ThreadCallback()` does not self-delete, `atbm_stopThreadInternal()` unconditionally force-deletes, and its return value is uninitialized.

### Task 2: Implement single-owner thread deletion

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_thread.c:13-87`
- Test: `tools/wireless_lib/tests/test_atbm_thread_shutdown.ps1`

- [x] **Step 1: Make the task wrapper self-delete after signaling completion**

Update `atbm_ThreadCallback()` so its declaration and final completion block are:

```c
static void atbm_ThreadCallback(void *arg){
    atbm_thread_internal_t *thread = (atbm_thread_internal_t *)arg;
    pAtbm_thread_t thread_id;
    int ret = 1;

    wifi_printk(WIFI_ALWAYS, "Thread %s %d start\n", thread->name, thread->wake_period);
    if(thread->wake_period){
        while (!thread->stop)
        {
            do{
                if(atbm_atomic_read(&thread->count) != 0)
                    break;
                ret = atbm_os_wait_event_timeout(&thread->wq, thread->wake_period);
            }while(0);
            atbm_atomic_set(&thread->count, 0);
            if(thread->cb){
                thread->cb(thread->p_arg, ret);
            }
        }
    }else{
        if(thread->cb){
            thread->cb(thread->p_arg, 0);
        }
    }

    thread_id = thread->thread;
    wifi_printk(WIFI_ALWAYS, "Thread %s stop\n", thread->name);
    atbm_os_wakeup_event(&thread->stopEv);
    atbm_ThreadStopEvent(thread_id);
}
```

There must be no access to `thread` after `atbm_os_wakeup_event()`.

- [x] **Step 2: Force-delete only when cooperative shutdown times out**

Replace `atbm_stopThreadInternal()` with:

```c
int atbm_stopThreadInternal(atbm_thread_internal_t *thread){
    int ret;

    thread->stop = 1;
    if(thread->wake_period){
        atbm_os_wakeup_event(&thread->wq);
    }
    ret = atbm_os_wait_event_timeout(&thread->stopEv, 10*ATBM_HZ);
    if(ret == 0){
        atbm_stopThread(thread->thread);
    }

    atbm_os_delete_waitevent(&thread->stopEv);
    if(thread->wake_period){
        atbm_os_delete_waitevent(&thread->wq);
    }

    atbm_kfree(thread);
    return (ret == 0) ? -1 : 0;
};
```

- [x] **Step 3: Run the focused test and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_thread_shutdown.ps1
```

Expected: `PASS: ATBM FreeRTOS thread shutdown contract` and exit code 0.

### Task 3: Verify P2P and public API compatibility

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_thread.c`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/p2p/p2p_main.c`

- [x] **Step 1: Run the existing P2P contract**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1
```

Expected: `PASS: ATBM P2P and application API contract` and exit code 0.

- [x] **Step 2: Run the public API compile contract**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/wireless_lib/tests/test_atbm_public_api.ps1
```

Expected: `PASS: ATBM public API headers compile without private include paths` and exit code 0.

- [x] **Step 3: Audit task-deletion ownership**

Run:

```powershell
rg -n -S 'atbm_ThreadStopEvent|atbm_os_wait_event_timeout|atbm_stopThread\(' `
  gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_thread.c
```

Expected: one normal self-delete call, one wait, and one force-delete call guarded by the timeout branch.

### Task 4: Attempt the focused ATBM library build

**Files:**
- Build: `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library_Debug.mk`

- [x] **Step 1: Check build tool availability without cleaning outputs**

Run:

```powershell
make --version
arm-none-eabi-gcc --version
```

Expected: both commands exit 0. If either tool is unavailable, record that the source-contract and API tests passed but do not claim the static library was rebuilt.

- [x] **Step 2: Build incrementally when tools are available**

Run from `Project/WirelessLibraries/ATBM_WiFi_Library`:

```powershell
make -f ATBM_WiFi_Library_Debug.mk
```

Expected: exit code 0 and an updated `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`.

### Task 5: Record version-control limitation

**Files:**
- No production files changed in this task.

- [x] **Step 1: Do not attempt commits in this workspace**

`git rev-parse --show-toplevel` has already returned `fatal: not a git repository`. Preserve unrelated edits, including the current `192.168.43.1` netif default, and report all changed files without committing.
