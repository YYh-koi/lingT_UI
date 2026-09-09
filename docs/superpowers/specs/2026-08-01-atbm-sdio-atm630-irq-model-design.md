# ATBM SDIO ATM630-Style IRQ Model Design

## Goal

Make the Generalplus ATBM SDIO interrupt path follow the proven ATM630
ownership model while retaining the Generalplus controller-specific register
operations.

## Design

- Replace the one-entry CMSIS message queue with a FreeRTOS counting semaphore
  whose maximum count is 32.
- The Generalplus SDIO ISR masks/clears the host interrupt through the existing
  `drvl1_sdc_*` path and gives the semaphore with the ISR-safe FreeRTOS API.
- The dedicated SDIO IRQ thread takes the semaphore, claims the SDIO host,
  reads CCCR `INTx`, dispatches the matching function callback, releases the
  host, and re-enables the Generalplus host interrupt at one common exit.
- The platform IRQ thread is the only normal owner of host IRQ re-enable. The
  ATBM RX task and ATBM callback early-return paths no longer re-enable it.
- Keep the existing 100 ms RX timeout recovery and bounded diagnostics during
  hardware validation.

## Error Handling

CCCR read failure, zero pending, unmatched pending, and missing handler all
flow through the same host-release and IRQ-re-enable exit. If the semaphore is
full, the ISR records the failure and leaves the IRQ masked; an already queued
worker wake owns the subsequent re-enable, avoiding an interrupt storm.
Teardown terminates the worker before deleting the semaphore.

## Verification

A focused structural regression check enforces the counting-semaphore model
and single re-enable owner. Existing private-SDIO checks and the dedicated
ATBM WiFi archive build verify integration. Final proof requires device testing
with reconnect/disconnect traffic and the SDIO diagnostic counters.
