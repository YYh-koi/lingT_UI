# SCO Downlink Audio Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make CVSD SCO downlink play as continuous 8 kHz mono signed 16-bit PCM without replaying erroneous SCO payloads.

**Architecture:** Keep the shared `bt_audio_task`, replace packet-slot buffering with one static FreeRTOS stream buffer, and select a smaller DMA transfer for 8 kHz calls. Configure and stop call playback from typed HFP AUDIO events, while the HFP SCO packet handler replaces payloads carrying a non-zero Packet Status with equal-length silence.

**Tech Stack:** C99, FreeRTOS static stream buffers/tasks, BTstack HCI/HFP, Generalplus DAC double-buffer DMA, PowerShell contract tests, ARM GCC 4.9.3.

---

## File map

- Create `Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1`: 8 kHz, byte-stream buffering, dynamic DMA and diagnostics contract.
- Create `Project/wifi_demo/tests/test_bt_xstack_demo_sco_contract.ps1`: typed HFP AUDIO lifecycle contract.
- Create `btstack_api/api/tests/test_bt_xstack_sco_packet_contract.ps1`: SCO header length/status and bad-packet silence contract.
- Modify `Project/wifi_demo/bt_audio_task.c`: static stream buffer, byte prebuffer, 8 kHz DMA sizing and diagnostics.
- Modify `Project/wifi_demo/bt_xstack_demo.c`: establish SCO on active call; configure/stop playback on AUDIO event.
- Modify `btstack_api/src/hci.h`: expose a bounded SCO Packet Status accessor.
- Modify `btstack_api/src/classic/hfp_hf.c`: validate SCO payload length and silence non-zero-status payloads.

### Task 1: Accept and buffer SCO PCM by bytes

**Files:**
- Create: `Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1`
- Modify: `Project/wifi_demo/bt_audio_task.c`

- [ ] **Step 1: Write the failing audio-task contract**

Create a PowerShell test that reads `bt_audio_task.c` and requires:

```powershell
$source = Get-Content -Raw $sourcePath
foreach ($required in @(
    '#include "stream_buffer.h"',
    'xStreamBufferCreateStatic',
    'xStreamBufferSend',
    'xStreamBufferReceive',
    'xStreamBufferBytesAvailable',
    'xStreamBufferReset',
    'BT_AUDIO_SCO_DMA_BUFFER_BYTES',
    'sample_rate == 8000u')) {
    if ($source -notmatch [regex]::Escape($required)) {
        throw "Missing SCO byte-stream behavior: $required"
    }
}
if ($source -match 'BT_AUDIO_PREBUFFER_CHUNKS|uxQueueMessagesWaiting') {
    throw 'Audio prebuffer still depends on packet count'
}
if ($source -notmatch '8000u.*44100u.*48000u') {
    throw 'Audio task does not accept SCO and A2DP sample rates'
}
if ($source -notmatch 'bt_audio_(drop|underflow)_count.*32u') {
    throw 'Audio diagnostics are not rate limited'
}
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_audio_task_sco_contract.ps1
```

Expected: FAIL containing `Missing SCO byte-stream behavior`.

- [ ] **Step 3: Replace packet queues with a static stream buffer**

In `bt_audio_task.c`, include `stream_buffer.h`, remove the chunk/free/ready queue types and state, and define:

```c
enum {
    BT_AUDIO_A2DP_DMA_BUFFER_BYTES = 4096,
    BT_AUDIO_SCO_DMA_BUFFER_BYTES = 512,
    BT_AUDIO_DMA_BUFFER_BYTES = BT_AUDIO_A2DP_DMA_BUFFER_BYTES,
    BT_AUDIO_DMA_BUFFER_SAMPLES = BT_AUDIO_DMA_BUFFER_BYTES / 2,
    BT_AUDIO_PCM_BUFFER_BYTES = BT_AUDIO_DMA_BUFFER_BYTES * 4,
    BT_AUDIO_TASK_STACK_WORDS = 1024
};

static StaticStreamBuffer_t bt_audio_pcm_stream_state;
static uint8_t bt_audio_pcm_stream_storage[BT_AUDIO_PCM_BUFFER_BYTES + 1u];
static StreamBufferHandle_t bt_audio_pcm_stream;
```

Create it in `bt_audio_task_start()` with a trigger level of one byte:

```c
bt_audio_pcm_stream = xStreamBufferCreateStatic(
    sizeof(bt_audio_pcm_stream_storage),
    1u,
    bt_audio_pcm_stream_storage,
    &bt_audio_pcm_stream_state);
if (bt_audio_pcm_stream == NULL) return -1;
```

- [ ] **Step 4: Make DMA filling length-aware**

Change the fill helper and DAC starter to accept active byte size:

```c
static void bt_audio_task_fill_dma(INT16S *buffer, uint32_t buffer_bytes)
{
    size_t received = xStreamBufferReceive(
        bt_audio_pcm_stream, (uint8_t *)buffer, buffer_bytes, 0u);
    if (received < buffer_bytes) {
        memset((uint8_t *)buffer + received, 0, buffer_bytes - received);
        bt_audio_underflow_count++;
        if ((bt_audio_underflow_count % 32u) == 1u) {
            DBG_PRINT("bt audio: underflow=%u missing=%u\r\n",
                (unsigned int)bt_audio_underflow_count,
                (unsigned int)(buffer_bytes - received));
        }
    }
}
```

For 8000 Hz select 512 bytes; otherwise select 4096 bytes. Pass
`buffer_bytes / sizeof(INT16S)` to both DAC double-buffer functions. Require
`xStreamBufferBytesAvailable() >= buffer_bytes * 2u` before starting, because
startup fills both DMA buffers.

- [ ] **Step 5: Accept 8 kHz and write non-blocking bytes**

Change validation to:

```c
if ((bt_audio_task_handle == NULL) ||
    ((sample_rate != 8000u) &&
     (sample_rate != 44100u) &&
     (sample_rate != 48000u)) ||
    ((channel_count != 1u) && (channel_count != 2u)) ||
    (bits_per_sample != 16u)) return -1;
```

Require mono for 8000 Hz. Implement writes with one zero-timeout stream-buffer
send, count unsent bytes, log one out of every 32 drops, and notify the audio
task. Reset the stream buffer when stopping or applying a new generation.

- [ ] **Step 6: Run the focused contract and verify GREEN**

Run the Task 1 command again.

Expected: `SCO byte-stream audio task contract is present`.

### Task 2: Drive call playback from HFP AUDIO state

**Files:**
- Create: `Project/wifi_demo/tests/test_bt_xstack_demo_sco_contract.ps1`
- Modify: `Project/wifi_demo/bt_xstack_demo.c`

- [ ] **Step 1: Write the failing Demo lifecycle contract**

Extract `bt_xstack_demo_phone_event` and require:

```powershell
if ($phoneCallback -notmatch 'BT_XSTACK_PHONE_EVENT_AUDIO') {
    throw 'Demo does not handle typed HFP AUDIO events'
}
if ($phoneCallback -notmatch 'values\[0\].*bt_audio_task_configure\s*\(\s*8000u\s*,\s*1u\s*,\s*16u\s*\)') {
    throw 'SCO connect does not configure 8 kHz mono PCM'
}
if ($phoneCallback -notmatch 'BT_XSTACK_PHONE_EVENT_AUDIO[\s\S]*bt_audio_task_stop\s*\(') {
    throw 'SCO release does not stop playback'
}
$stateBranch = [regex]::Match($phoneCallback,
    '(?s)BT_XSTACK_PHONE_EVENT_STATE.*?(?=BT_XSTACK_PHONE_EVENT_RING)').Value
if ($stateBranch -match 'bt_audio_task_configure') {
    throw 'Call state configures DAC before SCO is connected'
}
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_sco_contract.ps1
```

Expected: FAIL containing `Demo does not handle typed HFP AUDIO events`.

- [ ] **Step 3: Separate call and SCO lifecycle actions**

Keep `bt_xstack_hfp_set_audio_connected(true)` on transition to ACTIVE, but
remove the premature `bt_audio_task_configure()` call from that branch. Add a
phone-event branch:

```c
} else if ((event->id == BT_XSTACK_PHONE_EVENT_AUDIO) &&
           (event->value_count > 0u)) {
    uint8_t audio_connected = event->values[0] != 0u;
    taskEXIT_CRITICAL();
    if (audio_connected) {
        int dac_status = bt_audio_task_configure(8000u, 1u, 16u);
        DBG_PRINT("xstack demo call dac cfg status=%d\r\n", dac_status);
        if (dac_status != 0) bt_audio_task_stop();
    } else {
        bt_audio_task_stop();
    }
    taskENTER_CRITICAL();
```

Preserve stop behavior for call release, HFP LOST and power-off events.

- [ ] **Step 4: Run both Demo contracts and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_sco_contract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: both PASS.

### Task 3: Silence erroneous SCO packets

**Files:**
- Create: `btstack_api/api/tests/test_bt_xstack_sco_packet_contract.ps1`
- Modify: `btstack_api/src/hci.h`
- Modify: `btstack_api/src/classic/hfp_hf.c`

- [ ] **Step 1: Write the failing SCO packet contract**

Require a two-bit status accessor, payload-length validation, and silence before
callback:

```powershell
if ($hciHeader -notmatch 'READ_SCO_PACKET_STATUS.*>>\s*12.*0x03') {
    throw 'Missing bounded SCO Packet Status accessor'
}
foreach ($required in @(
    'payload_length = packet[2]',
    'size < (uint16_t)(HCI_SCO_HEADER_SIZE + payload_length)',
    'READ_SCO_PACKET_STATUS(packet)',
    'memset(&packet[HCI_SCO_HEADER_SIZE], 0, payload_length)',
    'hfp_hf_direct_audio_callback(&packet[HCI_SCO_HEADER_SIZE], payload_length)')) {
    if ($handler -notmatch [regex]::Escape($required)) {
        throw "Missing safe SCO delivery behavior: $required"
    }
}
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File btstack_api/api/tests/test_bt_xstack_sco_packet_contract.ps1
```

Expected: FAIL containing `Missing bounded SCO Packet Status accessor`.

- [ ] **Step 3: Add the status accessor and safe delivery**

In `hci.h` add:

```c
#define READ_SCO_PACKET_STATUS(buffer) \
    ((little_endian_read_16((buffer), 0) >> 12) & 0x03u)
```

In `hfp_hf_sco_packet_handler`, reject truncated payloads, use `packet[2]` as
the callback length, and overwrite only the SCO payload with zeros when status
is non-zero. Do not allocate memory or block.

- [ ] **Step 4: Run the SCO contract and API contracts**

Run the SCO packet contract, then all scripts under `btstack_api/api/tests`.

Expected: all PASS.

### Task 4: Build and regression verification

**Files:**
- Verify: `Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1`
- Verify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Build and deliver the API library**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Project/WirelessLibraries/BTstack_API_Library/build_btstack_api.ps1
```

Expected: exit 0 and refreshed
`gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`.

- [ ] **Step 2: Generate and build the Debug application**

Generate `GPCV2178_turnkey_sdk_release_Debug.mk` with `gpcmprj2make.exe`, then
run `mingw32-make.exe -j4` from `Project/GPCV2178_Turnkey_SDK`.

Expected: exit 0 and refreshed `prjDebug/bin/Debug/project.elf`.

- [ ] **Step 3: Run focused and existing regression contracts**

Run the three new tests, the existing Demo contract, copy-isolation contract,
and every `btstack_api/api/tests/test_*.ps1` script.

Expected: all PASS.

- [ ] **Step 4: Inspect the final ELF**

Use `arm-none-eabi-nm` and require `bt_xstack_init`,
`bt_xstack_hfp_set_audio_connected`, `hfp_hf_set_direct_audio_callback`, and
`xStreamBufferSend`; reject `bt_xstack_send_at`.

- [ ] **Step 5: Record the non-Git checkpoint**

Since this SDK directory is not a Git worktree, write SHA256 hashes for all
modified production/test files and the generated library/ELF to
`tmp/sco_downlink_fix_manifest.xml` using `Export-Clixml`.

