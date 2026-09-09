# TFT Register Readback and RGB Pattern Test Plan

**Goal:** Distinguish a framebuffer/programming failure from a TFT panel, timing, pinmux, or backlight failure.

**Architecture:** Keep interrupt work minimal: the VBlank ISR stores the return value from `drv_l2_tft_buffer_set()` and immediately reads the programmed framebuffer register through `drv_l2_tft_buffer_get()`. The statistics task prints those sampled values. On Bluetooth music entry, submit one full-screen red/green/blue vertical-band frame through the same JPEG display-message path used by Cover Art; the next Cover Art frame replaces it normally.

### Task 1: Lock the source contract

- Extend the focused PowerShell test to require buffer-set status, register readback, and the RGB test-pattern path.
- Run it once and confirm it fails before production edits.

### Task 2: Add ISR-safe register diagnostics

- Add global sampled status/readback fields in `main.c`.
- In each active VBlank update path, store `drv_l2_tft_buffer_set()` and `drv_l2_tft_buffer_get()` results without UART output in the ISR.
- Print both fields from the existing statistics task.

### Task 3: Add the RGB display-path test

- Replace the initial black Bluetooth-music frame with a three-band RGB frame.
- Encode the bands for GP420, RGB565, and YUYV; use the active panel dimensions/format.
- Drain the CPU cache, submit through `MSG_DISPLAY_TASK_JPEG_DRAW`, log the result, and return the frame on submission failure.

### Task 4: Verify and build

- Run the focused test and confirm it passes.
- Build the existing Debug make target.
- Verify the new strings in the ELF and report the generated BIN timestamp, size, and SHA-256.
