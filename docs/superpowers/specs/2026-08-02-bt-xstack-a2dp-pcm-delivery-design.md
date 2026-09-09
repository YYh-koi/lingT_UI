# xStack A2DP PCM Delivery and Playback Demo Design

## Goal

Complete the current A2DP Sink receive path so `libxstack.a` converts incoming
A2DP SBC media into decoded PCM, exposes the PCM through one minimal callback,
and provides a platform-side `bt_audio_task` demo that plays the PCM through
the GPCV2178 DAC and speaker.

This work is for A2DP playback verification. Production ownership of the DAC,
audio focus, mixing, and arbitration with other platform audio remains an
upper-layer responsibility.

## Current State

The current receive path reaches the registered internal AVDTP media handler:

```text
UART0 RX DMA -> H5 -> HCI ACL -> L2CAP -> AVDTP media channel
-> btstack_a2dp_media_handler()
```

`btstack_classic_profiles_setup()` zero-initializes the A2DP application
configuration and does not set `media_data_callback` or
`audio_sample_callback`. As a result, the current internal media handler drops
each media packet. No RTP/SBC depacketization, SBC decoding, PCM callback, or
DAC playback is currently active.

BTstack already emits the internal
`A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION` event, but the xStack
A2DP wrapper does not currently process it or expose the negotiated audio
configuration.

## Public API

Keep the existing AT callback and AT command APIs unchanged:

```c
int bt_xstack_init(bt_event_callback_t event_callback, void *context);
int bt_xstack_send_at(const char *command, uint16_t length);
```

Add the following A2DP PCM API to the application-facing `bt_xstack.h`:

```c
typedef void (*bt_xstack_pcm_callback_t)(
    const uint8_t *data,
    uint32_t len);

int bt_xstack_a2dp_register_pcm_callback(
    bt_xstack_pcm_callback_t callback);
```

The callback intentionally contains only `data` and `len`. Audio format
information is reported through the existing AT event output path.

### PCM Contract

- `data` contains decoded signed 16-bit little-endian PCM.
- Stereo samples are interleaved in `L, R, L, R` order.
- `len` is the number of valid PCM bytes, not the number of samples or frames.
- The buffer remains valid only for the duration of the callback.
- The application must copy the data promptly and must not block in the
  callback.
- The callback execution context is xStack-owned and is not part of the public
  ABI. The application must not assume that it runs in a particular task.
- Passing `NULL` unregisters the callback. If no callback is registered,
  A2DP signaling and streaming remain operational and decoded PCM is dropped.
- Calling the setter before `bt_xstack_init()` succeeds returns
  `BT_XSTACK_STATUS_NOT_INITIALIZED`. Registration and replacement after
  initialization are atomic with respect to PCM delivery.

### Configuration Event Contract

xStack parses the BTstack SBC codec configuration event and reports the
result through the existing `bt_event_callback_t` AT event path:

```text
+A2DPPCMCONFIG=<sample_rate_hz>,<channels>,<bits_per_sample>\r\n
```

For example:

```text
+A2DPPCMCONFIG=44100,2,16\r\n
```

- `sample_rate_hz` and `channels` reflect the negotiated SBC stream.
- `bits_per_sample` is always `16` for the public PCM output.
- The event is emitted once after every successful SBC configuration and
  again after a successful stream reconfiguration.
- The event is not emitted for every media packet.
- A malformed or unsupported SBC configuration does not emit the event and
  does not produce PCM.
- The AT output callback remains a byte-span callback; the application must
  use its explicit length rather than assume NUL termination.
- The platform demo may receive and buffer PCM before its AT event task has
  consumed the configuration event, but it must not start the DAC until the
  event has supplied a valid configuration.

## xStack Media Processing

The completed library data path is:

```text
AVDTP media packet
-> validate and parse RTP header
-> parse SBC media payload header
-> identify complete SBC frames
-> decode SBC frames to PCM S16
-> bt_xstack_pcm_callback_t(data, len)
```

The RTP parser accounts for the RTP header length, CSRC list, extension, and
padding before locating the SBC media payload header. Malformed packets are
dropped without reading outside the received buffer.

The initial implementation supports the normal complete SBC media packets
used by standard phones. A fragmented SBC media payload is rejected and
counted rather than passed partially to the decoder. Packet loss, malformed
RTP, unsupported fragmentation, SBC decode failure, and missing PCM consumer
are tracked with rate-limited diagnostics so media traffic cannot flood the
serial log.

Use an SBC decoder implementation matching the BTstack version already in the
tree and include the required decoder objects in `libxstack.a`. Decoder state
is reset when a stream is established, reconfigured, released, or
disconnected. The decoder never exposes SBC or RTP structures through the
public API.

The first implementation may decode in the existing xStack media execution
path. The public callback contract remains independent of that detail, so the
decoder can later move to a dedicated worker without changing the application
API. The platform callback must therefore remain short and non-blocking.

## AT Control

Playback control remains separate from PCM delivery:

```text
Control/config: AT commands and `+A2DPPCMCONFIG` -> bt_event_callback_t
Audio:          decoded PCM -> bt_xstack_pcm_callback_t(data, len)
```

No A2DP start, pause, stop, DAC, or speaker control function is added to the
PCM interface. The existing AT commands and events continue to control and
report playback. This preserves the current application control model.

## Platform Playback Demo

Add an application-owned `bt_audio_task` demo. It is not part of
`libxstack.a` and does not become a general platform audio manager.

The demo performs the following sequence:

1. Initialize a bounded PCM buffer and create `bt_audio_task`.
2. Register its PCM ingress callback after `bt_xstack_init()` succeeds.
3. In the PCM callback, copy available bytes into the bounded buffer and
   return immediately.
4. Parse `+A2DPPCMCONFIG` in the existing AT event callback and deliver the
   resulting format to `bt_audio_task`.
5. Configure sample rate and mono/stereo mode only after the configuration
   event has been received.
6. Prebuffer enough PCM for two DAC DMA blocks.
7. Feed the platform DAC using its double-buffer DMA interface.
8. Return completed DMA buffers to the free pool from the DMA completion path.
9. On demo pause or stop, stop feeding new buffers, clear buffered PCM, stop
   or mute the DAC, and release the speaker output used by the demo.

The demo uses a bounded buffer sized for approximately 100-150 ms of the
largest supported PCM stream. On overflow it drops the newest bytes and
increments an overflow counter. On underflow it submits silence for the next
DMA block and increments an underflow counter. These counters are printed at a
low rate for board validation.

The demo directly exercises the DAC only to prove the A2DP path. It does not
define how Bluetooth audio competes with WAV playback, prompts, HFP audio, or
other product audio sources.

## Delivery Layout

The internal source header remains under `btstack/include/adapter/` for
library compilation. The application-facing copy under
`gplib/liba_wifi_src/atbm/atbm6165/bt/` is updated with the same public PCM
types and functions, consistent with the existing formal delivery layout.

All new public names use the `bt_xstack` prefix and contain no ATBM-specific
symbol names.

## Error Handling

- A `NULL` PCM callback remains the valid unregister operation.
- An unsupported SBC configuration suppresses both PCM delivery and the
  `+A2DPPCMCONFIG` event for that stream.
- Malformed or unsupported media packets are dropped without stopping the
  H5, HCI, L2CAP, or AVDTP links.
- SBC decoder failures reset only media decoder state and are counted.
- Platform buffer overflow or underflow does not block the BTstack run loop.
- Stream release, disconnect, or reconfiguration clears stale decoder and
  demo PCM state before more audio is accepted.

## Verification

### Host and Contract Tests

- Verify the public header declares the callback with exactly `data` and
  `len` and declares the setter.
- Verify the internal and delivered public headers stay synchronized.
- Test callback registration, replacement, and `NULL` unregistration.
- Test exact `+A2DPPCMCONFIG` formatting after initial negotiation and
  reconfiguration, including `44100,2,16` and `48000,2,16` cases.
- Test RTP/SBC depacketization with valid packets and malformed lengths,
  CSRC, extension, padding, and fragmented payload flags.
- Decode a known SBC vector and verify PCM byte count, channel order, and a
  stable output checksum or reference sample sequence.
- Verify that missing callbacks and decode failures do not stop A2DP state
  progression.
- Verify demo buffer overflow, underflow, pause, clear, and DMA refill logic.

### Build Verification

- Build `libxstack.a` and confirm the new PCM setter symbol is exported.
- Build the main project with the `bt_audio_task` demo enabled.
- Confirm the final ELF contains the xStack PCM setter, SBC decoder,
  demo task, and DAC DMA calls.
- Run the existing Bluetooth profile, AT, transport, and build contract tests.

### Board Verification

1. Boot the board and wait for WiFi-triggered xStack startup.
2. Pair and connect a phone to `GP-BT-ATBM6165` as an A2DP Sink.
3. Start playback through the existing AT control path.
4. Confirm one valid `+A2DPPCMCONFIG` event is reported before DAC playback.
5. Confirm PCM byte counters increase without sustained overflow or
   underflow.
6. Confirm clean audio is heard through the speaker at the negotiated sample
   rate and channel count.
7. Pause and resume through AT commands and confirm PCM/DAC behavior follows.
8. Disconnect and reconnect, then confirm decoder and buffer state restart
   cleanly.

Successful completion requires both deterministic host/build checks and
audible board playback; entering A2DP `STREAMING` alone is not sufficient.
