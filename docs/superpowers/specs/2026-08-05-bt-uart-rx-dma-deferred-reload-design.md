# Bluetooth UART0 RX DMA Deferred Reload Design

## Goal

Keep the UART0 RX DMA block size at 512 bytes and make continuous H5 reception survive every full-block boundary during A2DP streaming.

## Root Cause

The current UART port calls `drv_l1_dma_transfer_double_buf_set()` from the DMA completion callback. The callback runs before the shared DMA interrupt handler clears the channel pending bit. After the first 512-byte block, software reports `running=1` and the transfer count is reloaded to 512, but DMA does not drain UART0. UART0 then reports RX full and overrun.

## Scope

- Modify only `btstack/ports/gpcv2178/btstack_uart_gpcv2178.c` and its focused tests.
- Keep `BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE` at `512u`.
- Do not modify the shared DMA driver, H5 protocol, public Bluetooth API, or UART pin/baud settings.
- Keep the existing UART health diagnostics enabled during validation.

## Design

### DMA completion ISR

On a successful full-block completion, the ISR will:

1. account for the completed 512 bytes;
2. queue the completed block for SLIP/H5 consumption;
3. mark hardware DMA as not currently transferring;
4. set a dedicated `reload_pending` flag;
5. wake the BTstack run loop;
6. return without calling `drv_l1_dma_transfer_double_buf_set()`.

The occupied DMA channel remains allocated. `reload_pending` is distinct from the existing `restart_pending`, which represents allocation of a new DMA channel.

### Run-loop reload

At the beginning of `btstack_uart_h5_rx_process_ready()`, before parsing the completed block, the run loop will handle `reload_pending`:

1. find a free RX block;
2. update the existing occupied DMA structure with that block;
3. call `drv_l1_dma_transfer_double_buf_set()` after the DMA IRQ has returned and its pending bit has been cleared;
4. mark DMA running only after the call succeeds;
5. reset the remaining-count/progress snapshot.

Reload occurs before SLIP/H5 parsing to minimize the time UART0 is not being drained.

### Error recovery

- If no free block is available, retain `reload_pending`, process/release queued data, and retry reload immediately afterward.
- If `drv_l1_dma_transfer_double_buf_set()` fails, log the channel and status once, release the occupied channel, and schedule the existing full `arm_new()` recovery path.
- Never leave `running=1` after a failed reload.

## Validation

### Stage A: UART-only isolation

Keep `BT_AUDIO_DIAG_BYPASS_DAC_DMA=1` and stream music. Success requires:

- `full_irq` increases beyond 1;
- `bytes` and `frames` continue increasing;
- `drop=0` and `error=0`;
- `rxerr` does not acquire the UART overrun bit `0x08`;
- `bt audio diag: DAC DMA bypassed` appears, proving PCM reached the application.

### Stage B: Audio restoration

Only after Stage A succeeds, set `BT_AUDIO_DIAG_BYPASS_DAC_DMA=0`, rebuild, and verify audible playback while the UART counters continue progressing without overrun.

## Tests and Build

- Add a source-contract test that fails while the ISR still calls the reload API directly.
- Require a distinct deferred reload flag, run-loop-first reload, 512-byte configuration, and failure recovery.
- Run the UART health, DAC isolation, and HCI dump tests.
- Rebuild `libxstack.a`, `project.elf`, and `project.bin` for each validation stage.

