# Bluetooth UART RX DMA Preloaded Double-Buffer Design

## Goal

Keep the UART0 H5 RX block size at 512 bytes while ensuring that a second DMA block is already queued before the current block completes.

## Evidence

The deferred-reload build still stops after `full_irq=1`. Its software state reports `running=1` and `dma_rem=512`, but UART0 reports RX full and overrun. `drv_l1_dma_transfer_double_buf_set()` only writes the next source, target, and count; it does not re-enable a DMA channel that already stopped. Working SDK users start the first DMA block and preload the second block before starting the producer.

## State Model

- `channel_owned`: the double-buffer DMA channel is allocated.
- `running`: hardware has a current block transferring.
- `active_block`: the block that will complete next.
- `queued_block`: the block already loaded into the hardware double-buffer slot.
- `reload_pending`: the running DMA needs another queued block.
- `restart_pending`: hardware stopped and the run loop must release the old channel and allocate a new one.

DMA-owned blocks use the existing block state `1`; completed blocks use state `2`; free blocks use state `0`.

## Startup and Normal Flow

1. Allocate and start `active_block` with `drv_l1_dma_transfer_double_buf_with_callback()`.
2. Immediately choose a second free block and call `drv_l1_dma_transfer_double_buf_set()` while the first block is still active.
3. On completion, the ISR queues `active_block`, promotes `queued_block` to `active_block`, clears `queued_block`, sets `reload_pending`, and wakes the run loop.
4. The run loop preloads another free block before parsing completed H5 data.
5. If no block is free, it parses/releases completed data and retries while the current 512-byte block remains active.

## Recovery and Cleanup

- If preloading fails, preserve the active transfer, record the error, and let the current block finish. With no queued successor, completion transitions to `restart_pending`.
- If a block completes without a queued successor, the ISR queues the completed block and marks hardware stopped. The run loop releases the occupied channel and starts a fresh active-plus-queued pair.
- Idle flush stops the channel, releases the unused queued block, queues only the received portion of the active block, then starts a fresh pair.
- Close releases the channel and both DMA-owned block states.

## Diagnostics and Validation

Extend the UART health line with channel ownership, queued block, hardware DBF state, and preload-pending state. Keep DAC DMA bypass enabled for UART-only validation.

Successful board validation requires:

- `full_irq` grows beyond 1;
- `bytes` and `frames` continue growing during A2DP streaming;
- `dbf=1` during continuous reception except for short refill windows;
- `drop=0`, `error=0`, and no UART overrun bit `0x08`;
- PCM reaches the existing DAC-bypass application callback.

## Scope

Modify only the GPCV2178 BTstack UART port, focused source-contract tests, and diagnostic expectations. Do not change the shared DMA driver, public Bluetooth API, baud rate, H5 protocol, or the 512-byte block size.
