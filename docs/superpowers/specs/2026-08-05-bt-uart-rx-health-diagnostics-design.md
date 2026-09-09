# Bluetooth UART0 RX Health Diagnostics Design

## Goal

Determine whether ATBM6165 stops transmitting H5 data during A2DP playback, or whether bytes reach GPCV2178 UART0 but stall in DMA, DMA IRQ, or SLIP/H5 processing.

## Scope

- Add diagnostics only in the GPCV2178 BTstack UART port.
- Keep UART0 RX DMA, H5 reliability, A2DP, and AT behavior unchanged.
- Keep the DAC DMA isolation switch enabled for this diagnostic build.
- Keep full HCI dump disabled.

## Diagnostics

When `BTSTACK_GPCV2178_UART_DIAG_VERBOSE` is enabled, a run-loop timer prints one health record per second containing:

- cumulative bytes accepted from UART0 RX DMA;
- completed SLIP/H5 frame count;
- current DMA channel and remaining transfer count;
- DMA running state, active block, ready block count, and current consumer offset/length;
- full-block DMA interrupt count, idle-flush count, drop count, and DMA error count;
- raw UART0 status and RX error registers.

The timer only reads registers and counters. It must not read `R_UART0_DATA`, clear UART status, reset DMA, or re-arm DMA.

## Interpretation

- Bytes and frames both stop; DMA remaining stays at 512; UART RX is empty: no new bytes are observed at GPCV2178 UART0, so the controller/physical RX path is the leading cause.
- Bytes stop; UART RX is non-empty/full or RX overrun is set: bytes reached UART0 but RX DMA is not draining them.
- Bytes increase while frames do not: DMA receives data, but SLIP/H5 framing does not complete.
- DMA remaining reaches zero while full-block interrupt count does not increase: DMA completion interrupt/dispatch is stalled.
- Bytes and frames continue: UART0 RX and H5 framing are alive; investigate H5 sequence/ACK handling above the UART port.

## Verification

- Add a source-level contract test that first fails until the periodic timer, counters, register reads, and log fields exist.
- Run the existing UART/DAC isolation and HCI dump configuration tests.
- Rebuild `libxstack.a` and the main project ELF/BIN.
- Confirm the health-log format string is present in the built image.

