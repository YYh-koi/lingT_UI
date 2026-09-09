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
