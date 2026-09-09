# Bluetooth UART RX DMA Deferred Reload Implementation Plan

> Execute with focused TDD and keep the 512-byte RX DMA block size unchanged.

**Goal:** Move UART0 H5 RX DMA buffer reload out of the DMA completion callback and into BTstack run-loop context so continuous A2DP traffic crosses repeated 512-byte boundaries without UART overrun.

**Architecture:** The ISR queues the completed block, marks the allocated DMA channel as awaiting reload, and wakes the run loop. The run loop reloads the occupied channel before parsing queued H5 bytes; failure releases the channel and falls back to the existing new-channel restart path.

**Scope:** `btstack_uart_gpcv2178.c`, one focused PowerShell source-contract test, existing diagnostic tests, `libxstack.a`, and the A2DP PCM debug application artifacts.

## Task 1: Add the failing deferred-reload contract test

- Create `btstack/ports/gpcv2178/tests/test_bt_uart_rx_dma_deferred_reload.ps1`.
- Verify the 512-byte configuration, a distinct reload-pending state, no double-buffer reload API in the RX ISR, run-loop-first reload, recovery on reload failure, and close-path cleanup.
- Run the test and confirm it fails against the current ISR reload implementation.

## Task 2: Implement run-loop deferred reload

- Add and initialize `btstack_uart_h5_rx_dma_reload_pending`.
- Replace the ISR reload helper with a run-loop helper that reloads the already allocated channel.
- On a full DMA completion, queue the block, set `running=0` and `reload_pending=1`, then return from the ISR.
- Invoke deferred reload before parsing and retry immediately after queued blocks are released.
- Prevent the new-channel restart path from running while the old channel awaits reload.
- On reload failure, release the channel, clear reload state, and schedule existing restart recovery.
- Free an allocated channel on close even during the deferred-reload window.

## Task 3: Verify and build

- Run the new contract test, UART health diagnostic test, DAC isolation test, and HCI dump test.
- Build `libxstack.a`.
- Build `project.elf` and generate `project.bin`.
- Confirm Stage A remains configured with 512-byte RX blocks and DAC DMA bypass enabled; runtime playback validation remains a board-side step.
