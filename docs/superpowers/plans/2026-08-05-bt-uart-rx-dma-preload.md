# Bluetooth UART RX DMA Preloaded Double-Buffer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep UART0 H5 RX continuously draining across repeated 512-byte DMA completions by preloading the hardware double-buffer slot before each active block finishes.

**Architecture:** Track the allocated channel, active block, and preloaded queued block independently. Startup creates an active-plus-queued pair; the ISR only rotates completed/queued state, while the BTstack run loop refills the now-empty hardware queue and performs any full-channel recovery.

**Tech Stack:** C, GPCV2178 DMA/UART drivers, BTstack run loop, PowerShell source-contract tests, GNU Arm Embedded toolchain.

---

### Task 1: Add failing preloaded-double-buffer contract tests

**Files:**
- Create: `btstack/ports/gpcv2178/tests/test_bt_uart_rx_dma_preload.ps1`
- Modify: `btstack/ports/gpcv2178/tests/test_bt_uart_rx_health_diagnostics.ps1`

- [ ] Assert that channel ownership and a distinct queued block are tracked.
- [ ] Assert that startup invokes the preload helper after the first DMA starts.
- [ ] Assert that the ISR promotes `queued_block` to `active_block` without calling DMA driver APIs.
- [ ] Assert that idle flush and close release queued-block ownership.
- [ ] Assert that health diagnostics expose `owned`, `queued`, `dbf`, and `preload`.
- [ ] Run the tests and confirm failure because the current implementation has no queued-block state.

### Task 2: Implement active/queued DMA rotation

**Files:**
- Modify: `btstack/ports/gpcv2178/btstack_uart_gpcv2178.c`

- [ ] Add `channel_owned` and `queued_block` state plus a task-context preload helper.
- [ ] Start the first block and immediately preload the second block.
- [ ] Change the RX DMA ISR to queue the completed active block and promote the queued block.
- [ ] Refill the hardware queue from the run loop before H5 parsing and after block release.
- [ ] When no successor was preloaded, defer channel release and full restart to the run loop.
- [ ] Make idle flush and close release both active and queued DMA ownership safely.

### Task 3: Verify and build Stage A firmware

**Files:**
- Verify: `btstack/ports/gpcv2178/btstack_config.h`
- Build: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf`
- Build: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`

- [ ] Run the preload, deferred-reload, UART health, DAC isolation, and HCI dump tests.
- [ ] Confirm `BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE=512u` and `BT_AUDIO_DIAG_BYPASS_DAC_DMA=1`.
- [ ] Build `libxstack.a` using `Xstack_Library_Debug.mk`.
- [ ] Build `project.elf` using `GPCV2178_A2dpPcm_Debug.mk` and generate `project.bin` with `arm-none-eabi-objcopy`.
- [ ] Confirm the new preload diagnostic strings are linked into the final ELF.
