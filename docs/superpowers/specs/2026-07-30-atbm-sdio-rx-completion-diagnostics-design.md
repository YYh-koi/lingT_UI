# ATBM SDIO RX Completion Diagnostics Design

## Goal

Determine whether the final SDIO interrupt stalls inside the independent RX
worker, is re-enabled but lost by the Generalplus controller/GIC, or is never
asserted by the ATBM LMAC after the phone transmits Deauth.

## Evidence

The `2026-07-30 21:46:17` serial log ends with SDIO interrupt 63. The interrupt
callback entry counter also reaches 63, but all RX/WSM counters then remain
unchanged for about 39 seconds. With `PROCESS_RX_IN_IRQ_THREAD=0`, callback
entry does not prove that the RX worker returned or re-enabled the host
interrupt.

## Considered Approaches

1. **Stage and state instrumentation (selected).** Record RX worker entry,
   descriptor wait/read/process stage, worker exit, interrupt re-enable, SDIO
   callback return, and periodic controller/CCCR state. This isolates the
   failing boundary without changing disconnect behavior.
2. **Force interrupt re-enable from a watchdog.** This may hide the failure and
   can race the RX worker, so it is not suitable until the blocked stage is
   proven.
3. **Switch RX processing into the IRQ thread.** This changes scheduling and
   timing substantially and would make comparison with ATM630 less useful.

## Design

### RX worker diagnostics

Add counters to `struct atbmwifi_common` for RX worker entry, exit, and
successful return from the host interrupt enable call. Add a stage value and
last return value:

- `1`: RX task entered
- `2`: waiting for the first RX descriptor
- `3`: first RX descriptor obtained
- `4`: reading the device RX length
- `5`: processing an RX buffer
- `6`: waiting for another descriptor indicated by piggyback length
- `7`: RX queue processing returned
- `8`: host SDIO interrupt re-enabled

Expose these fields in the existing five-second `[APDIAG]` counter line.

### SDIO callback completion

After the registered ATBM interrupt callback returns, print
`[SDIO-THREAD-DONE]` with controller interrupt-enable and pending state. This
distinguishes callback return from RX worker completion; in the current
asynchronous configuration the controller may still be disabled at this point.

### Idle controller/CCCR snapshot

Change the SDIO IRQ queue wait from infinite to a five-second diagnostic
timeout. On timeout, read:

- Generalplus controller interrupt-enable state
- Generalplus controller SDIO pending state
- SDIO CCCR INTx pending byte through CMD52
- ISR/thread/handler counters

Print the values as `[SDIO-IDLE]`. The snapshot is observational only: it does
not invoke the handler, clear software state, force re-enable, or synthesize a
disconnect.

## Interpretation

- RX task entry greater than exit, with stage 2 or 6: descriptor wait stalled.
- RX task entry greater than exit at stage 4 or 5: SDIO read/RX processing
  stalled.
- RX task exit equals entry but re-enable is behind: interrupt re-enable path
  stalled.
- RX task completed, CCCR INTx is `0x2`, and no ISR arrives: Generalplus
  controller/GIC missed the asserted function interrupt.
- RX task completed and CCCR INTx is zero: ATBM LMAC did not queue a host RX
  indication for the air Deauth.

## Verification

The source check must require all new counters and log markers. Rebuild all 96
ATBM WiFi objects, relink the current Debug firmware without `libbtstack.a`,
then verify `[SDIO-RX-TASK]`, `[SDIO-THREAD-DONE]`, and `[SDIO-IDLE]` are
present in `libatbm_wifi.a`, `project.elf`, and `project.bin`.
