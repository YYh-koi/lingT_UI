#ifndef BT_XSTACK_PHONE_H
#define BT_XSTACK_PHONE_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_XSTACK_PHONE_EVENT_STATE = 0,
    BT_XSTACK_PHONE_EVENT_DEVICE,
    BT_XSTACK_PHONE_EVENT_AUDIO,
    BT_XSTACK_PHONE_EVENT_VOICE_RECOGNITION,
    BT_XSTACK_PHONE_EVENT_SIGNAL,
    BT_XSTACK_PHONE_EVENT_NETWORK,
    BT_XSTACK_PHONE_EVENT_ROAM,
    BT_XSTACK_PHONE_EVENT_BATTERY,
    BT_XSTACK_PHONE_EVENT_NUMBER,
    BT_XSTACK_PHONE_EVENT_IN_BAND_RING,
    BT_XSTACK_PHONE_EVENT_RING,
    BT_XSTACK_PHONE_EVENT_LOST,
    BT_XSTACK_PHONE_EVENT_MULTIPARTY,
    BT_XSTACK_PHONE_EVENT_MICROPHONE_MUTE
} bt_xstack_phone_event_id_t;

typedef struct {
    bt_xstack_phone_event_id_t id;
    bt_xstack_addr_t address;
    /*
     * STATE: values[0] is bt_xstack_hfp_state_t.
     * DEVICE and LOST: no value fields are defined; address is the peer.
     * AUDIO, VOICE_RECOGNITION, NETWORK, ROAM, IN_BAND_RING, RING and
     * MICROPHONE_MUTE: values[0] is 0 (inactive) or 1 (active).
     * SIGNAL and BATTERY: values[0] is the native HFP indicator level.
     * NUMBER: text is the remote number; native supplementary values, when
     * present, remain in values[]. MULTIPARTY uses the native HFP values.
     */
    uint32_t values[2];
    uint8_t value_count;
    char text[BT_XSTACK_TEXT_MAX];
} bt_xstack_phone_event_t;

typedef void (*bt_xstack_phone_callback_t)(
    const bt_xstack_phone_event_t *event,
    void *context);

bt_xstack_result_t bt_xstack_get_hfp_config(uint8_t *config);
bt_xstack_result_t bt_xstack_set_hfp_config(uint8_t config);
bt_xstack_result_t bt_xstack_get_hfp_state(bt_xstack_hfp_state_t *state);
bt_xstack_result_t bt_xstack_hfp_dial(const char *number);
bt_xstack_result_t bt_xstack_hfp_redial(void);
bt_xstack_result_t bt_xstack_hfp_send_dtmf(char code);
bt_xstack_result_t bt_xstack_hfp_answer(void);
bt_xstack_result_t bt_xstack_hfp_hang_up(void);
bt_xstack_result_t bt_xstack_hfp_multiparty_control(
    bt_xstack_hfp_multiparty_action_t action);
bt_xstack_result_t bt_xstack_hfp_toggle_audio(void);
bt_xstack_result_t bt_xstack_hfp_set_audio_connected(bool connected);
bt_xstack_result_t bt_xstack_hfp_set_voice_recognition(bool enabled);
bt_xstack_result_t bt_xstack_get_microphone_mute(bool *muted);
bt_xstack_result_t bt_xstack_set_microphone_mute(bool muted);
bt_xstack_result_t bt_xstack_get_speaker_volume(uint8_t *volume);
bt_xstack_result_t bt_xstack_set_speaker_volume(uint8_t volume);

#ifdef __cplusplus
}
#endif

#endif
