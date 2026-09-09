# ATBM SDIO Disconnect Diagnostics Design

## Goal

Determine why a phone-to-AP Deauth/Disassoc frame visible over the air does not produce any host-side disconnect log.

## Evidence

- The current firmware reaches AP association and DHCP normally.
- No `[WSM-RX-MGMT]`, `[RX-DISPATCH-MGMT]`, or AP disconnect log appears during the reported disconnect.
- The embedded ATBM6165 LMAC image is byte-identical to the ZhengXin ATM630 reference.
- Host RX-filter initialization and update logic match the ZhengXin reference.
- The remaining platform-specific boundary is the Generalplus SDIO interrupt and receive path.

## Selected Approach

Add diagnostics only; do not change interrupt, RX-filter, inactivity, or disconnect behavior.

1. Enable the existing five-second AP link/RX counter report.
2. Add bounded logs at the Generalplus SDIO controller ISR and IRQ thread.
3. Preserve the existing WSM and management-dispatch logs.
4. Build the ATBM WiFi archive and main firmware, then verify all diagnostic strings are present in the output binary.

## Expected Diagnostic Boundary

- No `[SDIO-ISR]`: the Generalplus controller did not deliver the SDIO interrupt.
- `[SDIO-ISR]` but no `[SDIO-THREAD]`: the IRQ queue/thread path lost the event.
- `[SDIO-THREAD] pending=0`: the controller interrupted but the card CCCR INTx did not identify function 1.
- SDIO counters advance but WSM counters do not: the packet was read but not accepted as a WSM message.
- WSM counters and `[WSM-RX-MGMT]` advance: continue diagnosis above WSM.

## Noise and Safety

- Detailed ISR/thread logs are capped at 128 events.
- Five-second AP counters continue only while an AP station entry is active.
- No recovery, polling fallback, forced Deauth, or RX-filter override is introduced.

