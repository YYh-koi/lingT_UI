# SCO Downlink Stability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dedicated 2-KB SCO byte ring to `bt_audio_task`, preserve 16-bit PCM alignment under overflow, and start SCO playback with two 384-byte DMA buffers while leaving A2DP decoding unchanged.

**Architecture:** The existing A2DP path keeps its 16-KB FreeRTOS StreamBuffer. The direct SCO application callback writes complete chunks into a statically allocated ring; the audio task consumes only even byte counts, drops oldest even bytes on overflow, and fills missing DMA bytes with silence. Explicit media/call write APIs prevent stale A2DP PCM from entering the SCO path during source changes.

**Tech Stack:** C, FreeRTOS task notifications/critical sections/StreamBuffer, GPCV2178 DAC DMA, PowerShell contract tests, ARM GCC static-library and firmware builds.

---

## File map

- Modify `Project/Include/bt_audio_task.h`: source enum, diagnostics type, source-specific writers, diagnostics Getter.
- Modify `Project/wifi_demo/bt_audio_task.c`: SCO ring, source lifecycle, source-specific DMA reads, diagnostics.
- Modify `Project/wifi_demo/bt_xstack_demo.c`: route media and call PCM callbacks to explicit writers; remove high-rate callback logging.
- Create `Project/wifi_demo/tests/test_bt_audio_task_sco_ring_contract.ps1`: structural contract for the ring, 48-ms prebuffer, source routing and diagnostics.
- Modify `Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1`: replace the shared-StreamBuffer SCO expectations with the dedicated-ring contract.
- Modify `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`: require explicit media/call writer use and public header declarations.
- Modify `docs/superpowers/specs/2026-08-10-sco-downlink-stability-design.md`: change status to implemented only after verification succeeds.

This workspace has no Git metadata, so commit steps are replaced by test/build checkpoints and a SHA-256 verification manifest.

### Task 1: Add failing SCO ring contracts

**Files:**
- Create: `Project/wifi_demo/tests/test_bt_audio_task_sco_ring_contract.ps1`
- Modify: `Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1`
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Write a contract for the dedicated SCO ring**

Require the following concrete production symbols and behavior:

```powershell
$required = @(
    'BT_AUDIO_SCO_RING_BYTES = 2048',
    'BT_AUDIO_SCO_DMA_BUFFER_BYTES = 384',
    'BT_AUDIO_SCO_START_BYTES = 768',
    'bt_audio_sco_ring_storage',
    'bt_audio_sco_ring_read',
    'bt_audio_sco_ring_write',
    'bt_audio_sco_ring_count',
    'bt_audio_task_write_media_pcm',
    'bt_audio_task_write_call_pcm',
    'bt_audio_task_get_diagnostics')
```

Extract `bt_audio_task_write_call_pcm()`and require that it does not call `xStreamBufferSend`, does reject source mismatch, computes an even discard count with `(discard_bytes + 1u) & ~1u`, advances the oldest read index, writes the complete input, and calls `xTaskNotifyGive`.

- [ ] **Step 2: Update existing Demo expectations**

Require these exact calls in `bt_xstack_demo.c`:

```c
bt_audio_task_write_media_pcm(data, length);
bt_audio_task_write_call_pcm(data, length);
```

Require all four declarations in the public Demo audio header:

```c
bt_audio_task_write_pcm
bt_audio_task_write_media_pcm
bt_audio_task_write_call_pcm
bt_audio_task_get_diagnostics
```

- [ ] **Step 3: Run the focused tests and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Project\wifi_demo\tests\test_bt_audio_task_sco_ring_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Project\wifi_demo\tests\test_bt_audio_task_sco_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\Project\wifi_demo\tests\test_bt_xstack_demo_contract.ps1
```

Expected: failure because the dedicated ring constants, source-specific writers and diagnostics Getter do not exist.

### Task 2: Implement the dedicated SCO ring and diagnostics

**Files:**
- Modify: `Project/Include/bt_audio_task.h`
- Modify: `Project/wifi_demo/bt_audio_task.c`
- Test: `Project/wifi_demo/tests/test_bt_audio_task_sco_ring_contract.ps1`
- Test: `Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1`

- [ ] **Step 1: Define the application audio types and APIs**

Add these public Demo-layer types:

```c
typedef enum {
    BT_AUDIO_SOURCE_IDLE = 0,
    BT_AUDIO_SOURCE_A2DP,
    BT_AUDIO_SOURCE_SCO
} bt_audio_source_t;

typedef struct {
    bt_audio_source_t source;
    uint32_t sco_bytes;
    uint32_t sco_max_bytes;
    uint32_t sco_overflows;
    uint32_t sco_dropped_oldest_bytes;
    uint32_t source_mismatch_drops;
    uint32_t dma_underflows;
    uint32_t silence_bytes;
    uint32_t odd_tail_observations;
} bt_audio_task_diagnostics_t;
```

Declare the two explicit writers and synchronous Getter while retaining `bt_audio_task_write_pcm()`.

- [ ] **Step 2: Add static SCO state and reset/read helpers**

Add these fixed constants and objects:

```c
BT_AUDIO_SCO_DMA_BUFFER_BYTES = 384,
BT_AUDIO_SCO_START_BYTES = 768,
BT_AUDIO_SCO_RING_BYTES = 2048

static uint8_t bt_audio_sco_ring_storage[BT_AUDIO_SCO_RING_BYTES];
static uint16_t bt_audio_sco_ring_read;
static uint16_t bt_audio_sco_ring_write;
static uint16_t bt_audio_sco_ring_count;
static uint16_t bt_audio_sco_ring_max_count;
```

Implement reset inside a short critical section. Implement the SCO DMA reader so it copies at most `requested_bytes`, rounds available bytes down with `available &= ~1u`, advances the read index with wraparound, and fills the missing tail with zero after exiting the critical section.

- [ ] **Step 3: Implement complete SCO writes with oldest-even-byte overflow**

The writer must follow this algorithm under one bounded critical section:

```c
free_bytes = BT_AUDIO_SCO_RING_BYTES - bt_audio_sco_ring_count;
if (length > free_bytes) {
    discard_bytes = length - free_bytes;
    discard_bytes = (discard_bytes + 1u) & ~1u;
    bt_audio_sco_ring_read =
        (bt_audio_sco_ring_read + discard_bytes) %
        BT_AUDIO_SCO_RING_BYTES;
    bt_audio_sco_ring_count -= discard_bytes;
    bt_audio_sco_overflow_count++;
    bt_audio_sco_dropped_oldest_bytes += discard_bytes;
}
```

Reject `length > BT_AUDIO_SCO_RING_BYTES`; normal HCI SCO payloads are at most255 bytes. Copy the entire accepted input with wraparound, never a partial prefix. Increment `odd_tail_observations` when the committed ring count is odd, update the high-water mark, exit the critical section, then notify the task.

- [ ] **Step 4: Separate A2DP and SCO playback paths**

Maintain pending and active source state. During configure, set the write source to `IDLE` until the audio task has stopped DAC and reset both inputs. The audio task then publishes the new active source.

Use:

```c
source = (sample_rate == 8000u) ?
    BT_AUDIO_SOURCE_SCO : BT_AUDIO_SOURCE_A2DP;
```

For SCO, start only when ring bytes are at least `BT_AUDIO_SCO_START_BYTES`; fill/refill with384 bytes. For A2DP, retain the existing StreamBuffer,4,096-byte DMA blocks and two-block start threshold unchanged.

- [ ] **Step 5: Implement synchronous diagnostics**

Copy source, ring watermarks and all counters into the caller structure under one short critical section. Return `-1` for NULL and `0` for success. Do not submit xStack requests or touch the run-loop.

- [ ] **Step 6: Run focused tests and verify GREEN**

Run the two audio-task contract scripts. Expected: both pass with messages for dedicated SCO ring and byte-stream behavior.

### Task 3: Route Demo callbacks explicitly

**Files:**
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`
- Test: `Project/wifi_demo/tests/test_bt_xstack_demo_sco_contract.ps1`

- [ ] **Step 1: Change the media callback**

Replace the generic write with:

```c
bt_audio_task_write_media_pcm(data, length);
```

- [ ] **Step 2: Change the call callback**

Replace the generic write with:

```c
bt_audio_task_write_call_pcm(data, length);
```

Remove the per-50/per-100 callback `DBG_PRINT` counters so neither high-rate PCM callback performs formatting or UART output on the run-loop.

- [ ] **Step 3: Run Demo contracts**

Run all `Project/wifi_demo/tests/test_*.ps1`. Expected: three passes, including typed HFP AUDIO lifecycle and explicit PCM routing.

### Task 4: Regression, build and invariants

**Files:**
- Modify after success: `docs/superpowers/specs/2026-08-10-sco-downlink-stability-design.md`
- Create: `tmp/btstack_sco_stability_manifest.txt`

- [ ] **Step 1: Run complete control/API/Demo/SCO regression**

Run every `test_*.ps1` in `btstack_api/api/tests` and `Project/wifi_demo/tests`. Expected:18 passes after adding the new contract.

- [ ] **Step 2: Verify A2DP decode is unchanged**

Require `btstack_api/profiles/btstack_a2dp_media.c` still calls:

```c
btstack_sbc_decoder_process_data(
    &btstack_a2dp_media.decoder,
    0,
    packet + offset,
    (int)(payload_end - offset));
```

Confirm there is no new A2DP queue or media task symbol.

- [ ] **Step 3: Build the API library**

Run `Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1`. Expected: exit0; the existing `avdtp.c:339` warning may remain.

- [ ] **Step 4: Generate and link the Debug firmware**

Put `D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin` at the front of the current PowerShell `Path`, generate the project makefile, and run:

```powershell
D:\mingw32\bin\mingw32-make.exe `
  -f .\GPCV2178_turnkey_sdk_release_Debug.mk -j4
```

Expected: exit0 and `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.elf` exists.

- [ ] **Step 5: Verify linked symbols and static memory**

Use `arm-none-eabi-nm` to require `bt_audio_task_write_call_pcm`, `bt_audio_task_write_media_pcm`, and `bt_audio_task_get_diagnostics` in the ELF. Compare the map's relevant `.bss` objects and require exactly2,048 bytes for `bt_audio_sco_ring_storage` with no new task stack.

- [ ] **Step 6: Mark the spec implemented and write the manifest**

Only after all prior steps pass, update the spec status to implemented. Record test count, build results, ELF size and SHA-256 hashes for the modified sources, tests and ELF in `tmp/btstack_sco_stability_manifest.txt`.
