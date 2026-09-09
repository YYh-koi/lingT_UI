# Bluetooth UART0 RX Health Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add non-intrusive periodic UART0 RX/DMA diagnostics that distinguish missing ATBM6165 traffic from GPCV2178 DMA, IRQ, or H5 framing stalls.

**Architecture:** Extend only the GPCV2178 BTstack UART port. Count bytes at the two points where DMA ownership transfers to software, then use a one-second BTstack run-loop timer to read DMA remaining count and UART status/error registers and print one compact health record. Do not read UART data, clear status, reset DMA, or change H5 behavior.

**Tech Stack:** C, GPCV2178 driver_l1 UART/DMA registers, BTstack run-loop timers, PowerShell source-contract tests, ARM GCC make builds.

---

### Task 1: Add the failing diagnostic contract test

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_bt_uart_rx_health_diagnostics.ps1`
- Test: `btstack/ports/gpcv2178/tests/test_bt_uart_rx_health_diagnostics.ps1`

- [ ] **Step 1: Write the failing test**

Create a PowerShell test that loads `btstack_config.h` and `btstack_uart_gpcv2178.c`, then requires:

```powershell
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$config = Get-Content -Raw (Join-Path $root 'btstack/ports/gpcv2178/btstack_config.h')
$uart = Get-Content -Raw (Join-Path $root 'btstack/ports/gpcv2178/btstack_uart_gpcv2178.c')

if ($config -notmatch '#define\s+BTSTACK_GPCV2178_UART_HEALTH_PERIOD_MS\s+1000u') { throw 'Missing one-second UART health period' }
foreach ($token in @('bytes=%u','frames=%u','dma_rem=%u','full_irq=%u','idle_flush=%u','uart=0x%08x','rxerr=0x%08x')) {
    if ($uart -notmatch [regex]::Escape($token)) { throw "Missing UART health field: $token" }
}
if ($uart -notmatch 'btstack_uart_h5_rx_byte_count\s*\+=\s*BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE') { throw 'Full DMA blocks are not counted' }
if ($uart -notmatch 'btstack_uart_h5_rx_byte_count\s*\+=\s*received') { throw 'Idle-flushed DMA bytes are not counted' }
if ($uart -notmatch 'dma_get_tx_count') { throw 'DMA remaining count is not sampled' }
if (($uart -notmatch 'R_UART0_STATUS') -or ($uart -notmatch 'R_UART0_RX_STATUS')) { throw 'UART status registers are not sampled' }
if ($uart -notmatch 'btstack_run_loop_set_timer_handler\s*\(\s*&btstack_uart_h5_rx_health_timer') { throw 'Health timer is not registered' }
if ($uart -notmatch 'btstack_run_loop_remove_timer\s*\(\s*&btstack_uart_h5_rx_health_timer') { throw 'Health timer is not removed on close' }
Write-Host 'UART0 RX health diagnostics are configured'
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_bt_uart_rx_health_diagnostics.ps1
```

Expected: FAIL with `Missing one-second UART health period`.

### Task 2: Implement byte accounting and the periodic health record

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_config.h`
- Modify: `btstack/ports/gpcv2178/btstack_uart_gpcv2178.c`
- Test: `btstack/ports/gpcv2178/tests/test_bt_uart_rx_health_diagnostics.ps1`

- [ ] **Step 1: Add the diagnostic period**

Add beside the existing verbose UART switch:

```c
#define BTSTACK_GPCV2178_UART_HEALTH_PERIOD_MS 1000u
```

- [ ] **Step 2: Add counters and timer state**

Add a cumulative RX byte counter. Under `#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE`, add a run-loop timer and active flag:

```c
static volatile uint32_t btstack_uart_h5_rx_byte_count;
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
static btstack_timer_source_t btstack_uart_h5_rx_health_timer;
static uint8_t btstack_uart_h5_rx_health_timer_active;
#endif
```

- [ ] **Step 3: Count physically transferred bytes**

In the full-block DMA completion path, before queueing the block:

```c
btstack_uart_h5_rx_byte_count += BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
```

In the idle-flush path, after calculating a nonzero `received` count and before queueing:

```c
btstack_uart_h5_rx_byte_count += received;
```

- [ ] **Step 4: Add the read-only timer handler**

Under the verbose diagnostic switch, implement a scheduler and handler. Use `0xffffffffu` as the remaining-count sentinel when DMA is not running or cannot be read:

```c
static void btstack_uart_h5_rx_health_timer_schedule(void)
{
    if (!btstack_uart_rx_dma_enabled || btstack_uart_h5_rx_health_timer_active) return;
    btstack_run_loop_set_timer(&btstack_uart_h5_rx_health_timer,
        BTSTACK_GPCV2178_UART_HEALTH_PERIOD_MS);
    btstack_run_loop_add_timer(&btstack_uart_h5_rx_health_timer);
    btstack_uart_h5_rx_health_timer_active = 1u;
}

static void btstack_uart_h5_rx_health_timer_handler(btstack_timer_source_t *timer)
{
    INT32U remaining = 0xffffffffu;
    INT32U uart_status = R_UART0_STATUS;
    INT32U rx_error = R_UART0_RX_STATUS;
    (void)timer;
    btstack_uart_h5_rx_health_timer_active = 0u;
    if (btstack_uart_h5_rx_dma_running &&
        (dma_get_tx_count(btstack_uart_rx_dma.channel, &remaining) != 0)) {
        remaining = 0xffffffffu;
    }
    BTSTACK_UART_DIAG_PRINTF(
        "btstack uart rx health bytes=%u frames=%u dma_rem=%u dma_ch=%u running=%u block=%u ready=%u cur=%u/%u full_irq=%u idle_flush=%u drop=%u error=%u uart=0x%08x rxerr=0x%08x\r\n",
        (unsigned int)btstack_uart_h5_rx_byte_count,
        (unsigned int)btstack_uart_h5_rx_frame_count,
        (unsigned int)remaining,
        (unsigned int)btstack_uart_rx_dma.channel,
        (unsigned int)btstack_uart_h5_rx_dma_running,
        (unsigned int)btstack_uart_h5_rx_dma_active_block,
        (unsigned int)btstack_uart_h5_rx_ready_count,
        (unsigned int)btstack_uart_h5_rx_current_offset,
        (unsigned int)btstack_uart_h5_rx_current_len,
        (unsigned int)btstack_uart_h5_rx_block_count,
        (unsigned int)btstack_uart_h5_rx_idle_flush_count,
        (unsigned int)btstack_uart_h5_rx_drop_count,
        (unsigned int)btstack_uart_dma_error_count,
        (unsigned int)uart_status,
        (unsigned int)rx_error);
    btstack_uart_h5_rx_health_timer_schedule();
}
```

- [ ] **Step 5: Initialize, start, and stop the health timer**

In `open`, reset the byte counter and timer state, register the handler, and schedule it after UART setup. In `close`, remove it when active:

```c
btstack_uart_h5_rx_byte_count = 0u;
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
btstack_uart_h5_rx_health_timer_active = 0u;
btstack_run_loop_set_timer_handler(&btstack_uart_h5_rx_health_timer,
    btstack_uart_h5_rx_health_timer_handler);
btstack_uart_h5_rx_health_timer_schedule();
#endif
```

```c
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
if (btstack_uart_h5_rx_health_timer_active) {
    btstack_run_loop_remove_timer(&btstack_uart_h5_rx_health_timer);
    btstack_uart_h5_rx_health_timer_active = 0u;
}
#endif
```

- [ ] **Step 6: Run focused tests**

Run the new test plus:

```powershell
powershell -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\test_bt_audio_uart_dma_isolation.ps1
powershell -ExecutionPolicy Bypass -File btstack\ports\gpcv2178\tests\check_btstack_hci_dump_gpcv2178.ps1
```

Expected: all three exit with code 0.

### Task 3: Rebuild and verify deliverables

**Files:**
- Build: `Project/WirelessLibraries/BTstack_Library`
- Build: `Project/GPCV2178_Turnkey_SDK`
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Verify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Verify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] **Step 1: Build the static library**

Run from `Project/WirelessLibraries/BTstack_Library`:

```powershell
D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe -f ..\..\..\output\wireless_libs\build\Xstack_Library_Debug.mk debug
```

Expected: exit code 0 and an updated `libxstack.a`.

- [ ] **Step 2: Build the main project**

Run from `Project/GPCV2178_Turnkey_SDK`:

```powershell
D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\make.exe -f ..\..\output\wireless_libs\build\GPCV2178_A2dpPcm_Debug.mk debug
```

Expected: exit code 0; only the existing `PRINT_BUF_SIZE` redefinition warning is acceptable.

- [ ] **Step 3: Regenerate and verify the binary**

Run `arm-none-eabi-objcopy -O binary project.elf project.bin`, then use `arm-none-eabi-strings` to require the `btstack uart rx health` format string in both ELF and BIN.

- [ ] **Step 4: Report runtime interpretation**

Tell the user to play music and capture at least five consecutive health records. Interpret `bytes`, `frames`, `dma_rem`, `uart`, and `rxerr` according to the approved design.

