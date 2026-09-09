#ifndef BT_XSTACK_AUDIO_H
#define BT_XSTACK_AUDIO_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_XSTACK_AUDIO_EVENT_A2DP_STATE = 0,
    BT_XSTACK_AUDIO_EVENT_A2DP_DEVICE,
    BT_XSTACK_AUDIO_EVENT_A2DP_LOST,
    BT_XSTACK_AUDIO_EVENT_PCM_CONFIG,
    BT_XSTACK_AUDIO_EVENT_AVRCP_STATE,
    BT_XSTACK_AUDIO_EVENT_PLAY_STATE,
    BT_XSTACK_AUDIO_EVENT_PLAY_MODE,
    BT_XSTACK_AUDIO_EVENT_TRACK_STATE,
    BT_XSTACK_AUDIO_EVENT_TRACK_INFO,
    BT_XSTACK_AUDIO_EVENT_ABSOLUTE_VOLUME
} bt_xstack_audio_event_id_t;

typedef struct {
    bt_xstack_audio_event_id_t id;
    bt_xstack_addr_t address;
    /*
     * A2DP_STATE: values[0] is bt_xstack_a2dp_state_t.
     * A2DP_DEVICE and A2DP_LOST: no value fields are defined; address is peer.
     * PCM_CONFIG: values[0]=sample rate, [1]=channel count,
     * [2]=bits per sample.
     * AVRCP_STATE: values[0] is bt_xstack_avrcp_state_t.
     * PLAY_STATE and TRACK_STATE: values[0] is bt_xstack_play_state_t.
     * PLAY_MODE: values[0]=repeat mode and [1]=shuffle mode (native values).
     * TRACK_INFO: title, artist and album contain the metadata.
     * ABSOLUTE_VOLUME: values[0] is the native AVRCP volume (0..127).
     */
    uint32_t values[3];
    uint8_t value_count;
    char title[BT_XSTACK_TEXT_MAX];
    char artist[BT_XSTACK_TEXT_MAX];
    char album[BT_XSTACK_TEXT_MAX];
} bt_xstack_audio_event_t;

typedef void (*bt_xstack_audio_event_callback_t)(
    const bt_xstack_audio_event_t *event,
    void *context);
typedef void (*bt_xstack_media_audio_callback_t)(
    const uint8_t *data,
    uint32_t length,
    void *context);
typedef void (*bt_xstack_call_audio_callback_t)(
    const uint8_t *data,
    uint32_t length,
    void *context);

bt_xstack_result_t bt_xstack_get_a2dp_state(bt_xstack_a2dp_state_t *state);
bt_xstack_result_t bt_xstack_get_a2dp_mute(bool *muted);
bt_xstack_result_t bt_xstack_set_a2dp_mute(bool muted);
bt_xstack_result_t bt_xstack_get_avrcp_state(bt_xstack_avrcp_state_t *state);
bt_xstack_result_t bt_xstack_get_avrcp_play_state(
    bt_xstack_play_state_t *state);
bt_xstack_result_t bt_xstack_avrcp_play_pause(void);
bt_xstack_result_t bt_xstack_avrcp_play(void);
bt_xstack_result_t bt_xstack_avrcp_pause(void);
bt_xstack_result_t bt_xstack_avrcp_stop(void);
bt_xstack_result_t bt_xstack_avrcp_next(void);
bt_xstack_result_t bt_xstack_avrcp_previous(void);
bt_xstack_result_t bt_xstack_avrcp_fast_forward(void);
bt_xstack_result_t bt_xstack_avrcp_rewind(void);
bt_xstack_result_t bt_xstack_avrcp_set_repeat(bool enabled);
bt_xstack_result_t bt_xstack_avrcp_set_shuffle(bool enabled);
bt_xstack_result_t bt_xstack_avrcp_volume_up(void);
bt_xstack_result_t bt_xstack_avrcp_volume_down(void);
/* Non-blocking, all-or-nothing microphone/SCO uplink admission. */
bt_xstack_result_t bt_xstack_call_audio_write(
    const uint8_t *data,
    uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
