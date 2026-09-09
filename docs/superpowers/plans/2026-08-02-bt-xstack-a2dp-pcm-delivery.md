# BT xStack A2DP PCM Delivery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Decode A2DP Sink SBC media inside `libxstack.a`, deliver decoded PCM through an application-registered callback, report PCM format through `+A2DPPCMCONFIG`, and verify playback with a platform DAC demo.

**Architecture:** The profile layer parses RTP/SBC and owns the SBC decoder. It emits format changes through the existing AT event path and PCM bytes through one registered `data,len` callback bridged by `bt_xstack_manager.c`. The application callback only copies PCM into a bounded queue; a separate `bt_audio_task` owns DAC configuration, buffering, DMA refill, silence insertion, and shutdown.

**Tech Stack:** C, BTstack Classic/A2DP Sink, BlueKitchen Bluedroid SBC decoder, GPCV2178 FreeRTOS-style message queues/tasks, GPCV2178 DAC double-buffer API, PowerShell contract tests, G+IDE ARM toolchain.

---

## File map

- Modify `btstack/include/adapter/bt_xstack.h`: public PCM callback typedef and registration API.
- Modify `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`: delivered application header kept identical to the source public header.
- Modify `btstack/adapter/bt_xstack_manager.c`: callback state and internal sample-to-public-PCM bridge.
- Modify `btstack/profiles/btstack_a2dp_sink.c`: consume codec configuration events and route media packets to the focused media module.
- Modify `btstack/include/profiles/btstack_a2dp_sink.h`: structured PCM-configuration profile event.
- Create `btstack/profiles/btstack_a2dp_media.h`: private media module interface.
- Create `btstack/profiles/btstack_a2dp_media.c`: RTP/SBC validation, decoder lifecycle, and PCM sample delivery.
- Modify `btstack/at/btstack_at_internal.h`: internal `A2DPPCMCONFIG` event ID.
- Modify `btstack/at/btstack_at_backend.c`: profile-event to AT-event translation.
- Modify `btstack/at/btstack_at_protocol.c`: serialize `+A2DPPCMCONFIG=<rate>,<channels>,<bits>`.
- Add `btstack/3rd-party/bluedroid/decoder/`: pinned upstream OI SBC decoder sources and license files.
- Add decoder wrapper sources under `btstack/src/classic/` required by the existing SBC headers.
- Modify `btstack/adapter/Makefile.inc`: compile the new media module and decoder sources.
- Modify `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`: add sources and OI decoder include directory to `libxstack.a`.
- Create `Project/Include/bt_audio_task.h` and `Project/wifi_demo/bt_audio_task.c`: platform-only PCM queue and DAC playback worker.
- Modify `Project/wifi_demo/bt_xstack_demo.c`: register PCM callback, parse the format AT event, and start Bluetooth after Wi-Fi succeeds.
- Modify `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`: compile the platform playback demo.
- Extend the existing contract tests and add focused media/build-integration tests.

This workspace has no Git/SVN metadata and no `svn` executable, so each task ends with a test/build checkpoint instead of a commit. Do not overwrite unrelated local changes.

### Task 1: Add the registered PCM callback API

**Files:**
- Modify: `btstack/include/adapter/bt_xstack.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h`
- Modify: `btstack/adapter/bt_xstack_manager.c`
- Test: `btstack/adapter/tests/test_bt_xstack_manager_contract.ps1`

- [ ] **Step 1: Add failing public API contract checks**

Extend the PowerShell test to require the exact public contract in both headers and reject a PCM configuration getter:

```powershell
$headers = @(
    'btstack/include/adapter/bt_xstack.h',
    'gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h'
)
foreach ($header in $headers) {
    $text = Get-Content -Raw $header
    if ($text -notmatch 'typedef\s+void\s*\(\s*\*bt_xstack_pcm_callback_t\s*\)\s*\(\s*const\s+uint8_t\s*\*\s*data\s*,\s*uint32_t\s+len\s*\)') { throw "missing PCM callback in $header" }
    if ($text -notmatch 'int\s+bt_xstack_a2dp_register_pcm_callback\s*\(\s*bt_xstack_pcm_callback_t\s+callback\s*\)') { throw "missing PCM registration in $header" }
    if ($text -match 'bt_xstack_a2dp_get_pcm_config') { throw "PCM config getter is not part of the API" }
}
```

- [ ] **Step 2: Run the contract test and confirm RED**

Run:

```powershell
pwsh -NoProfile -File btstack/adapter/tests/test_bt_xstack_manager_contract.ps1
```

Expected: FAIL with `missing PCM callback`.

- [ ] **Step 3: Add the public type and registration function**

Add the following to the source header and copy the same declaration to the formal delivery header:

```c
typedef void (*bt_xstack_pcm_callback_t)(const uint8_t *data, uint32_t len);

/* callback == NULL unregisters the current sink. */
int bt_xstack_a2dp_register_pcm_callback(bt_xstack_pcm_callback_t callback);
```

In `bt_xstack_manager.c`, keep the application callback private and bridge the existing internal audio sample type:

```c
static bt_xstack_pcm_callback_t bt_xstack_pcm_callback;

static void bt_xstack_pcm_bridge(const btstack_audio_sample_t *sample)
{
    bt_xstack_pcm_callback_t callback = bt_xstack_pcm_callback;
    if ((callback == NULL) || (sample == NULL) ||
        (sample->format != BTSTACK_AUDIO_FORMAT_PCM_S16) ||
        (sample->data == NULL) || (sample->data_len == 0u)) {
        return;
    }
    callback(sample->data, sample->data_len);
}

int bt_xstack_a2dp_register_pcm_callback(bt_xstack_pcm_callback_t callback)
{
    if (!bt_xstack_initialized) {
        return BT_XSTACK_ERROR_NOT_INITIALIZED;
    }
    bt_xstack_pcm_callback = callback;
    return BT_XSTACK_OK;
}
```

Install `bt_xstack_pcm_bridge` once after `btstack_gpcv2178_init()` succeeds. Use the manager's existing critical-section convention when reading/writing the pointer; do not invoke the application callback while holding that critical section.

- [ ] **Step 4: Run the manager contract test and confirm GREEN**

Run the command from Step 2.

Expected: PASS, including identical public declarations in source and delivery headers.

- [ ] **Step 5: Record the checkpoint**

Save `git diff --no-index btstack/include/adapter/bt_xstack.h gplib/liba_wifi_src/atbm/atbm6165/bt/bt_xstack.h` output. Expected: no differences for the application-facing declarations.

### Task 2: Report PCM format through the AT event path

**Files:**
- Modify: `btstack/include/profiles/btstack_a2dp_sink.h`
- Modify: `btstack/profiles/btstack_a2dp_sink.c`
- Modify: `btstack/at/btstack_at_internal.h`
- Modify: `btstack/at/btstack_at_backend.c`
- Modify: `btstack/at/btstack_at_protocol.c`
- Modify: `btstack/at/tests/test_btstack_at_protocol.c`
- Modify: `btstack/at/tests/test_btstack_at_backend_contract.ps1`

- [ ] **Step 1: Add failing serialization and backend contract tests**

Add a protocol golden case that constructs an event with three numeric values:

```c
event.id = BTSTACK_AT_EVENT_A2DPPCMCONFIG;
event.value_count = 3;
event.values[0] = 44100;
event.values[1] = 2;
event.values[2] = 16;
CHECK_FORMAT(event, "+A2DPPCMCONFIG=44100,2,16\r\n");
```

Extend the backend contract test to require handling of `BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG` and population of all three values.

- [ ] **Step 2: Run both tests and confirm RED**

Run:

```powershell
pwsh -NoProfile -File btstack/at/tests/test_btstack_at_backend_contract.ps1
```

Then build/run `btstack/at/tests/test_btstack_at_protocol.c` using the command already documented at the top of that test file.

Expected: backend contract FAIL because the event IDs are absent; protocol compile FAIL on `BTSTACK_AT_EVENT_A2DPPCMCONFIG`.

- [ ] **Step 3: Add a structured profile event**

Add this event without exposing it through the public `bt_xstack` application header:

```c
typedef enum {
    /* existing values remain unchanged */
    BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG
} btstack_a2dp_sink_event_type_t;

typedef struct {
    btstack_a2dp_sink_event_type_t type;
    uint32_t sample_rate_hz;
    uint8_t channels;
    uint8_t bits_per_sample;
} btstack_a2dp_sink_pcm_config_event_t;
```

On `A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION`, read the generated BTstack getters `...get_sampling_frequency()` and `...get_num_channels()`, set bits to 16, configure the media decoder, then emit `BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG`. Emit once per accepted configuration; reset it when the stream is released or disconnected.

- [ ] **Step 4: Translate and serialize the AT event**

Add `BTSTACK_AT_EVENT_A2DPPCMCONFIG` to `btstack_at_internal.h`. In `btstack_at_backend_a2dp_event()` translate the profile event:

```c
at_event.id = BTSTACK_AT_EVENT_A2DPPCMCONFIG;
at_event.value_count = 3u;
at_event.values[0] = pcm->sample_rate_hz;
at_event.values[1] = pcm->channels;
at_event.values[2] = pcm->bits_per_sample;
btstack_at_backend_emit(&at_event);
```

Map the event ID to the exact name `A2DPPCMCONFIG` in `btstack_at_protocol.c`; use the existing numeric-event serializer so the bytes are exactly:

```text
+A2DPPCMCONFIG=<sample_rate>,<channels>,<bits>\r\n
```

- [ ] **Step 5: Run both tests and confirm GREEN**

Run the commands from Step 2.

Expected: backend contract PASS; protocol golden output equals `+A2DPPCMCONFIG=44100,2,16\r\n`.

- [ ] **Step 6: Record the checkpoint**

Run:

```powershell
rg -n "A2DPPCMCONFIG|PCM_CONFIG" btstack/include/profiles btstack/profiles btstack/at
```

Expected: one profile event producer, one backend translation, one protocol name, and the focused tests.

### Task 3: Vendor and compile the SBC decoder

**Files:**
- Create: `btstack/3rd-party/bluedroid/decoder/include/*.h`
- Create: `btstack/3rd-party/bluedroid/decoder/srce/*.c`
- Create/Modify: `btstack/src/classic/btstack_sbc_decoder_bluedroid.c`
- Create/Modify: `btstack/src/classic/btstack_sbc_plc.c`
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Create: `btstack/profiles/tests/test_btstack_sbc_build_integration.ps1`

- [ ] **Step 1: Add a failing build-integration test**

Create a PowerShell test that verifies every required source exists and is listed by the static-library project:

```powershell
$sources = @(
    'alloc.c','bitalloc.c','bitalloc-sbc.c','bitstream-decode.c',
    'decoder-oina.c','decoder-private.c','decoder-sbc.c','dequant.c',
    'framing.c','framing-sbc.c','oi_codec_version.c','synthesis-sbc.c',
    'synthesis-dct8.c','synthesis-8-generated.c'
)
$project = Get-Content -Raw 'Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj'
foreach ($source in $sources) {
    $path = Join-Path 'btstack/3rd-party/bluedroid/decoder/srce' $source
    if (-not (Test-Path $path)) { throw "missing $path" }
    if ($project -notmatch [regex]::Escape($source)) { throw "library omits $source" }
}
if ($project -notmatch '3rd-party[/\\]bluedroid[/\\]decoder[/\\]include') {
    throw 'library omits decoder include directory'
}
```

- [ ] **Step 2: Run the integration test and confirm RED**

Run:

```powershell
pwsh -NoProfile -File btstack/profiles/tests/test_btstack_sbc_build_integration.ps1
```

Expected: FAIL on the first missing decoder source.

- [ ] **Step 3: Import the pinned decoder implementation**

Copy the OI decoder headers and fourteen sources from the official BlueKitchen BTstack tree at commit `3bd65e43040e1e159988711f668c085ea3733b74`. Preserve copyright/license text and add the corresponding upstream license file. Import the matching `btstack_sbc_decoder_bluedroid.c` and `btstack_sbc_plc.c` implementations compatible with the existing `btstack_sbc.h`, `btstack_sbc_bluedroid.h`, and `btstack_sbc_plc.h` headers.

Do not modify decoder arithmetic or regenerate `synthesis-8-generated.c`; this task is a mechanical, pinned third-party import.

- [ ] **Step 4: Add decoder sources to both build descriptions**

Add the wrapper, PLC, and OI decoder `.c` files to `btstack/adapter/Makefile.inc` and `BTstack_Library.gpcmprj`. Add this include directory to the library compiler settings:

```text
../../../btstack/3rd-party/bluedroid/decoder/include
```

Keep encoder-only units out of the library because A2DP Sink playback only decodes SBC.

- [ ] **Step 5: Run the integration test and compile `libxstack.a`**

Run the test from Step 2, then:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$makefile = 'output\wireless_libs\build\BTstack_A2dpPcm_Debug.mk'
New-Item -ItemType Directory -Force -Path (Split-Path $makefile) | Out-Null
& "$toolDir\gpcmprj2make.exe" --local -in 'Project\WirelessLibraries\BTstack_Library\BTstack_Library.gpcmprj' -cfg Debug -out $makefile -tp $toolDir -windows
& "$toolDir\make.exe" -f $makefile
```

Expected: contract PASS; library build exits 0 and refreshes `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`.

- [ ] **Step 6: Record the checkpoint**

Record the imported upstream commit, file manifest, and `libxstack.a` size/timestamp in the implementation notes so future decoder upgrades remain auditable.

### Task 4: Parse A2DP media and decode SBC to PCM

**Files:**
- Create: `btstack/profiles/btstack_a2dp_media.h`
- Create: `btstack/profiles/btstack_a2dp_media.c`
- Modify: `btstack/profiles/btstack_a2dp_sink.c`
- Modify: `btstack/adapter/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Create: `btstack/profiles/tests/test_btstack_a2dp_media_contract.ps1`
- Modify: `btstack/profiles/tests/check_btstack_a2dp_profile.ps1`

- [ ] **Step 1: Add failing parser and lifecycle contract checks**

Create a contract test requiring these concrete protections and integration points:

```powershell
$media = Get-Content -Raw 'btstack/profiles/btstack_a2dp_media.c'
$sink = Get-Content -Raw 'btstack/profiles/btstack_a2dp_sink.c'
@('RTP_VERSION','RTP_PADDING','RTP_EXTENSION','RTP_CSRC_COUNT','SBC_FRAGMENTED') | ForEach-Object {
    if ($media -notmatch $_) { throw "missing media guard $_" }
}
if ($media -notmatch 'btstack_sbc_decoder_process_data') { throw 'decoder is not fed' }
if ($media -notmatch 'BTSTACK_AUDIO_FORMAT_PCM_S16') { throw 'PCM format is not emitted' }
if ($sink -notmatch 'btstack_a2dp_media_process') { throw 'sink media handler is not connected' }
if ($sink -notmatch 'btstack_a2dp_media_reset') { throw 'stream teardown does not reset decoder' }
```

- [ ] **Step 2: Run the test and confirm RED**

Run:

```powershell
pwsh -NoProfile -File btstack/profiles/tests/test_btstack_a2dp_media_contract.ps1
```

Expected: FAIL because `btstack_a2dp_media.c` does not exist.

- [ ] **Step 3: Define the private media module**

Use this private interface; it must not appear in the delivered application header:

```c
void btstack_a2dp_media_init(btstack_audio_sample_callback_t callback);
int btstack_a2dp_media_configure(uint32_t sample_rate_hz, uint8_t channels);
void btstack_a2dp_media_reset(void);
void btstack_a2dp_media_process(const uint8_t *packet, uint16_t size);
```

`configure` accepts only 44100/48000 Hz and mono/stereo for the first playback demo. It initializes the decoder and stores PCM metadata. `reset` clears decoder/configuration state and makes later packets no-ops until reconfigured.

- [ ] **Step 4: Implement bounded RTP/SBC parsing**

Implement these checks in order before accessing payload bytes:

```c
enum {
    RTP_FIXED_HEADER_SIZE = 12,
    RTP_VERSION = 2,
    RTP_PADDING = 0x20,
    RTP_EXTENSION = 0x10,
    RTP_CSRC_COUNT = 0x0f,
    SBC_FRAGMENTED = 0xe0
};
```

Require at least 13 bytes; require RTP version 2. Advance by `12 + 4 * CC`. If the extension bit is set, require its four-byte header and advance by `4 + big_endian_read_16(packet, offset + 2) * 4`. If padding is set, require a nonzero final padding count that fits inside the payload. Read the SBC media header, reject any fragmentation/start/last bit for the initial implementation, and require its low-nibble frame count to be nonzero. Pass only the remaining consecutive SBC bytes to `btstack_sbc_decoder_process_data()`.

On malformed or unsupported packets, increment a compact diagnostic counter and return; do not dump full ACL/media payloads.

- [ ] **Step 5: Deliver decoded PCM using the internal sample type**

In the decoder callback, calculate the byte length from decoded audio frames:

```c
sample.format = BTSTACK_AUDIO_FORMAT_PCM_S16;
sample.sample_rate_hz = media.sample_rate_hz;
sample.channels = media.channels;
sample.bits_per_sample = 16u;
sample.data = (const uint8_t *)pcm;
sample.data_len = (uint32_t)num_audio_frames * media.channels * sizeof(int16_t);
media.callback(&sample);
```

Confirm the imported wrapper's callback parameter represents audio frames before using this formula. If its documented unit is scalar samples, set `data_len = num_samples * sizeof(int16_t)` instead and add that unit to the function comment.

- [ ] **Step 6: Connect configuration, packets, and teardown**

Initialize the media module during A2DP Sink setup with the existing internal sample callback. Call `btstack_a2dp_media_configure()` only after an accepted SBC codec configuration event. Replace the old whole-packet `BTSTACK_AUDIO_FORMAT_SBC` callback with `btstack_a2dp_media_process(packet,size)`. Reset on media release, streaming connection release, and A2DP disconnect.

Add `btstack_a2dp_media.c` to both build descriptions.

- [ ] **Step 7: Run profile/media tests and rebuild the library**

Run:

```powershell
pwsh -NoProfile -File btstack/profiles/tests/test_btstack_a2dp_media_contract.ps1
pwsh -NoProfile -File btstack/profiles/tests/check_btstack_a2dp_profile.ps1
```

Then rerun the `libxstack.a` build from Task 3.

Expected: both tests PASS; library link has no unresolved SBC decoder symbols.

- [ ] **Step 8: Record the checkpoint**

Capture one test vector result showing a valid SBC payload produces nonzero PCM bytes, plus malformed RTP, fragmented SBC, and unconfigured-stream cases producing no callback.

### Task 5: Add the platform `bt_audio_task` playback demo

**Files:**
- Create: `Project/Include/bt_audio_task.h`
- Create: `Project/wifi_demo/bt_audio_task.c`
- Modify: `Project/wifi_demo/bt_xstack_demo.c`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`

- [ ] **Step 1: Add failing demo contract checks**

Extend the existing demo contract test to require registration order, exact event parsing, bounded-copy semantics, and task-owned DAC calls:

```powershell
$demo = Get-Content -Raw 'Project/wifi_demo/bt_xstack_demo.c'
$audio = Get-Content -Raw 'Project/wifi_demo/bt_audio_task.c'
if ($demo -notmatch 'bt_audio_task_start') { throw 'audio task is not started' }
if ($demo -notmatch 'bt_xstack_a2dp_register_pcm_callback\s*\(\s*bt_audio_task_write_pcm\s*\)') { throw 'PCM callback is not registered' }
if ($demo -notmatch '\+A2DPPCMCONFIG=') { throw 'PCM config event is not parsed' }
if ($demo -match 'strlen\s*\(\s*data\s*\)') { throw 'event callback must honor its byte length' }
if ($audio -notmatch 'memcpy') { throw 'PCM callback must copy data' }
if ($audio -notmatch 'drv_l1_dac_cha_dbf') { throw 'audio task does not drive DAC DBF' }
```

- [ ] **Step 2: Run the demo contract and confirm RED**

Run:

```powershell
pwsh -NoProfile -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: FAIL with `audio task is not started`.

- [ ] **Step 3: Define the platform audio-task boundary**

Create this application-only interface:

```c
int bt_audio_task_start(void);
void bt_audio_task_stop(void);
int bt_audio_task_configure(uint32_t sample_rate_hz,
                            uint8_t channels,
                            uint8_t bits_per_sample);
void bt_audio_task_write_pcm(const uint8_t *data, uint32_t len);
```

Allocate a fixed pool of thirty-two 512-byte PCM chunks and two 4096-byte aligned DMA buffers. `write_pcm` performs only bounded copies and nonblocking queue posts. Split callbacks larger than 512 bytes into chunks; drop the newest remainder when no free chunk is available and increment a drop counter.

- [ ] **Step 4: Implement task-owned DAC playback**

The task accepts only 44100/48000 Hz, one/two channels, and 16-bit PCM. On configuration change, stop any active DAC, flush queued PCM, set mono/stereo mode and `drv_l1_dac_sample_rate_set()`, then wait for data.

Prebuffer both 4096-byte DMA buffers. A 4096-byte buffer contains 2048 signed 16-bit DAC samples, so start double buffering with:

```c
drv_l1_dac_cha_dbf_put_callback(buffer0, 2048u, bt_audio_dac_isr);
drv_l1_dac_cha_dbf_set(buffer1, 2048u);
ap_peripheral_speaker_ctrl(1);
```

The ISR only posts a DMA-done message. The task refills and resubmits the completed buffer. Fill missing bytes with signed 16-bit silence on underflow. On stop, disable the speaker, call `drv_l1_dac_timer_stop()` and `drv_l1_dac_dbf_free()`, then return all queued chunks to the free pool.

This demo owns only validation playback; production audio focus and resource arbitration remain the upper layer's responsibility.

- [ ] **Step 5: Register the callback before enabling Bluetooth**

In `bt_xstack_demo_start()`, preserve the agreed order:

```c
status = bt_xstack_init(bt_xstack_demo_event, NULL);
if (status != BT_XSTACK_OK) return status;

status = bt_audio_task_start();
if (status != 0) return status;

status = bt_xstack_a2dp_register_pcm_callback(bt_audio_task_write_pcm);
if (status != BT_XSTACK_OK) return status;

return bt_xstack_send_at("AT+BTEN=1\r\n", sizeof("AT+BTEN=1\r\n") - 1u);
```

Keep `bt_xstack_demo_start()` called only after the existing Wi-Fi-success path.

- [ ] **Step 6: Parse the configuration event by byte span**

In the event callback, compare against the exact prefix within `data,len`, copy at most one short line into a local NUL-terminated buffer, and parse:

```c
if (sscanf(line, "+A2DPPCMCONFIG=%lu,%u,%u",
           &sample_rate, &channels, &bits) == 3) {
    bt_audio_task_configure((uint32_t)sample_rate,
                            (uint8_t)channels,
                            (uint8_t)bits);
}
```

Reject truncated input, extra numeric fields, unsupported values, and lines longer than the local buffer. Do not use `strlen(data)` because the callback already supplies the valid length.

- [ ] **Step 7: Add files to the main project and run the demo test**

Add `bt_audio_task.c` and its include path to the main `.gpcmprj`, then rerun Step 2.

Expected: PASS, with `AT+BTEN=1` still sent only after init, task start, and callback registration.

- [ ] **Step 8: Record the checkpoint**

Record the fixed demo memory budget: 16 KiB PCM chunk pool, 8 KiB DAC buffers, plus task stack/queue control structures.

### Task 6: Regression, firmware build, and board verification

**Files:**
- Verify: `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a`
- Verify: `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`
- Verify: serial log captured during phone-to-board A2DP playback

- [ ] **Step 1: Run all focused host-side contracts**

Run:

```powershell
pwsh -NoProfile -File btstack/adapter/tests/test_bt_xstack_manager_contract.ps1
pwsh -NoProfile -File btstack/at/tests/test_btstack_at_backend_contract.ps1
pwsh -NoProfile -File btstack/profiles/tests/test_btstack_sbc_build_integration.ps1
pwsh -NoProfile -File btstack/profiles/tests/test_btstack_a2dp_media_contract.ps1
pwsh -NoProfile -File btstack/profiles/tests/check_btstack_a2dp_profile.ps1
pwsh -NoProfile -File Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1
```

Expected: every script exits 0. Also build/run the AT protocol C test from Task 2 and require exact CRLF output.

- [ ] **Step 2: Build the final static library**

Run the Task 3 G+IDE library build.

Expected: exit 0; `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a` timestamp changes and contains the A2DP media/SBC decoder objects.

- [ ] **Step 3: Build the main firmware**

Run:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$makefile = 'output\wireless_libs\build\GPCV2178_A2dpPcm_Debug.mk'
& "$toolDir\gpcmprj2make.exe" --local -in 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj' -cfg Debug -out $makefile -tp $toolDir -windows
& "$toolDir\make.exe" -f $makefile
```

Expected: exit 0 and a refreshed `Project/GPCV2178_Turnkey_SDK/prjDebug/bin/Debug/project.bin`.

- [ ] **Step 4: Verify initialization and discovery on the board**

Flash the generated `project.bin` using the existing IDE download flow. Confirm serial output shows H5 link activation, `+PWRSTAT=1`, device name `GP-BT-ATBM6165`, and discoverable/connectable mode. Confirm no repeated logical HCI command appears before its completion event; H5 reliable transport retransmission remains allowed.

- [ ] **Step 5: Verify A2DP format and PCM delivery**

Pair a phone, start A2DP playback, and require exactly one configuration line for the negotiated stream, for example:

```text
+A2DPPCMCONFIG=44100,2,16
```

Confirm the PCM callback count and byte count increase, the callback drop counter remains zero during steady playback, and full ACL payload hex dumps are absent.

- [ ] **Step 6: Verify audible playback and controls**

Confirm clean audio from the board speaker for at least two minutes. Exercise existing AT play/pause commands and verify playback pauses/resumes without re-registering the callback. Disconnect/reconnect once and verify decoder/DAC state resets cleanly and a new PCM configuration event arrives.

- [ ] **Step 7: Verify failure behavior**

Stop the audio task while streaming and confirm PCM is safely dropped with no crash. Unregister via `bt_xstack_a2dp_register_pcm_callback(NULL)` and confirm decoding cannot call stale application memory. Re-register and reconnect to confirm recovery.

- [ ] **Step 8: Preserve verification evidence**

Save the final build logs, library/firmware timestamps, serial trace, negotiated format, callback/drop/underflow counters, and the exact flashed binary path with the implementation notes.
