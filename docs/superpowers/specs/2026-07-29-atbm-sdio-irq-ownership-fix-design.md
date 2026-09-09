# ATBM SDIO IRQ Ownership Fix Design

> Superseded on 2026-08-01 by
> `2026-08-01-atbm-sdio-atm630-irq-model-design.md` after comparison with the
> working ATM630 counting-semaphore interrupt path.

## Problem

The Generalplus SDIO interrupt thread disables the host IRQ, reads CCCR INTx,
calls the ATBM callback, and then unconditionally enables the host IRQ again.
With `PROCESS_RX_IN_IRQ_THREAD == 0`, the callback only wakes `Atbm_RX`; that
task drains RX and enables the host IRQ when it finishes. The two independent
enable paths race, producing duplicate/spurious interrupts, a full one-entry
IRQ queue, and the observed `zero pending` and `put_fail` counters.

## Design

Use a single-owner contract:

- after a matched SDIO function callback is invoked, the ATBM callback path
  owns host IRQ re-enable;
- the Generalplus SDIO thread re-enables the IRQ only when no callback handled
  the event, including zero-pending and unmatched-pending cases;
- CCCR INTx read failure keeps its existing recovery and re-enable path;
- both ATBM IRQ-processing variants explicitly re-enable the host IRQ before
  returning early for `!init_done` or `bh_error`;
- the asynchronous RX task remains the normal re-enable owner after it drains
  receive work.

The RTK 8189FTV SDIO implementation is not changed. Increasing the queue depth
is intentionally excluded because it would only hide the ownership race.

## Verification

Add a focused PowerShell structural regression test. Verify it fails against
the current unconditional platform re-enable, then passes after the ownership
change. Run the existing ATBM/private-SDIO checks and rebuild
`libatbm_wifi.a`. Runtime acceptance is that sustained traffic, reconnect, and
power-cycle tests no longer increase `put_fail`, while `zero pending` remains
zero or does not grow under normal traffic.
