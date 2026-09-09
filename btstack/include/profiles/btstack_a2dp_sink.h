#ifndef BTSTACK_PROFILE_A2DP_SINK_H
#define BTSTACK_PROFILE_A2DP_SINK_H

#include <stdbool.h>
#include <stdint.h>

#include "events/btstack_event.h"
#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTSTACK_A2DP_SINK_STATE_IDLE = 0,
    BTSTACK_A2DP_SINK_STATE_CONNECTING,
    BTSTACK_A2DP_SINK_STATE_CONNECTED,
    BTSTACK_A2DP_SINK_STATE_STREAM_OPEN,
    BTSTACK_A2DP_SINK_STATE_STREAMING,
    BTSTACK_A2DP_SINK_STATE_SUSPENDED
} btstack_a2dp_sink_state_t;

typedef enum {
    BTSTACK_A2DP_SINK_EVENT_DISCONNECTED = 0,
    BTSTACK_A2DP_SINK_EVENT_CONNECTING,
    BTSTACK_A2DP_SINK_EVENT_CONNECTED,
    BTSTACK_A2DP_SINK_EVENT_STREAM_OPEN,
    BTSTACK_A2DP_SINK_EVENT_STREAMING,
    BTSTACK_A2DP_SINK_EVENT_SUSPENDED,
    BTSTACK_A2DP_SINK_EVENT_STREAM_CLOSED,
    BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST,
    BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG
} btstack_a2dp_sink_event_type_t;

typedef struct {
    btstack_a2dp_sink_event_type_t type;
    btstack_bd_addr_t address;
    uint32_t sample_rate;
    uint8_t channel_count;
    uint8_t bits_per_sample;
} btstack_a2dp_sink_event_t;

typedef void (*btstack_a2dp_sink_event_callback_t)(
    const btstack_a2dp_sink_event_t *event);

typedef enum {
    BTSTACK_A2DP_SINK_CONTROL_PLAY = 0,
    BTSTACK_A2DP_SINK_CONTROL_PAUSE,
    BTSTACK_A2DP_SINK_CONTROL_STOP,
    BTSTACK_A2DP_SINK_CONTROL_NEXT,
    BTSTACK_A2DP_SINK_CONTROL_PREVIOUS,
    BTSTACK_A2DP_SINK_CONTROL_VOLUME_UP,
    BTSTACK_A2DP_SINK_CONTROL_VOLUME_DOWN
} btstack_a2dp_sink_control_t;

typedef struct {
    btstack_a2dp_sink_event_callback_t event_callback;
    btstack_audio_sample_callback_t audio_sample_callback;
    btstack_media_data_callback_t media_data_callback;
    const char *service_name;
    const char *manufacturer;
    uint32_t sdp_handle;
    uint16_t audio_buffer_ms;
} btstack_a2dp_sink_config_t;

/*
 * setup() initializes the native A2DP Sink and binds generic callbacks.
 * SBC media is decoded to PCM for audio_sample_callback. The optional raw
 * media callback still receives the original RTP/SBC packet.
 */
int btstack_a2dp_sink_setup(
    const btstack_a2dp_sink_config_t *config);
int btstack_a2dp_sink_connect(const btstack_bd_addr_t address);
int btstack_a2dp_sink_disconnect(void);
bool btstack_a2dp_sink_is_connected(void);
btstack_a2dp_sink_state_t btstack_a2dp_sink_get_state(void);
const uint8_t *btstack_a2dp_sink_get_peer_address(void);
int btstack_a2dp_sink_set_event_callback(
    btstack_a2dp_sink_event_callback_t callback);
int btstack_a2dp_sink_set_audio_sample_callback(
    btstack_audio_sample_callback_t callback);
int btstack_a2dp_sink_set_media_data_callback(
    btstack_media_data_callback_t callback);
int btstack_a2dp_sink_control(
    btstack_a2dp_sink_control_t control);

#ifdef __cplusplus
}
#endif

#endif
