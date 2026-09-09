# TFT VBlank Diagnostics Design

## Goal

Determine whether a decoded Bluetooth Cover Art framebuffer is consumed by the TFT VBlank path and passed to `drv_l2_tft_buffer_set()` without printing from interrupt context or changing display timing.

## Design

- Keep the existing JPEG receive and ready-queue logs.
- Record the most recently consumed `DISP_P` pointer and framebuffer base in `ap_display_vblank_isr()` immediately before the TFT buffer is selected.
- Reuse `fps_tft_update` as the raw number of VBlank frame updates in the current statistics interval.
- Print the raw update count and the last consumed frame/base from the existing statistics task before it resets the counters.
- Log the return status from `ap_display_queue_put(display_ready_queue, frame_get)` for JPEG submissions.

The ISR only performs scalar assignments; it does not call `DBG_PRINT`. Diagnostic output remains in task context.

## Success Criteria

The next board log must distinguish these cases:

1. Queue put fails: `put` is non-zero or `ready` is null.
2. Queue contains the frame but VBlank does not consume it: `update_raw=0` and last frame/base remain zero.
3. VBlank consumes and selects the framebuffer: `update_raw>0` and last frame/base match the queued frame and its base.

## Scope

This change adds diagnostics only. It does not alter BIP, JPEG decoding, TFT initialization, queue ordering, interrupt timing, or panel/backlight control.
