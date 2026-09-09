#ifndef BTSTACK_PROFILE_HFP_HF_H
#define BTSTACK_PROFILE_HFP_HF_H

#include <stdbool.h>
#include <stdint.h>

#include "events/btstack_event.h"
#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTSTACK_HFP_HF_CONTROL_ANSWER = 0,
    BTSTACK_HFP_HF_CONTROL_HANG_UP,
    BTSTACK_HFP_HF_CONTROL_REDIAL,
    BTSTACK_HFP_HF_CONTROL_RELEASE_HELD,
    BTSTACK_HFP_HF_CONTROL_HOLD_ACTIVE_ACCEPT_OTHER,
    BTSTACK_HFP_HF_CONTROL_JOIN_CALLS,
    BTSTACK_HFP_HF_CONTROL_CONNECT_AUDIO,
    BTSTACK_HFP_HF_CONTROL_DISCONNECT_AUDIO,
    BTSTACK_HFP_HF_CONTROL_ENABLE_VOICE_RECOGNITION,
    BTSTACK_HFP_HF_CONTROL_DISABLE_VOICE_RECOGNITION
} btstack_hfp_hf_control_t;

#define BTSTACK_HFP_HF_TEXT_MAX 64u

typedef enum {
    BTSTACK_HFP_HF_STATE_IDLE = 1,
    BTSTACK_HFP_HF_STATE_CONNECTING,
    BTSTACK_HFP_HF_STATE_CONNECTED,
    BTSTACK_HFP_HF_STATE_OUTGOING,
    BTSTACK_HFP_HF_STATE_INCOMING,
    BTSTACK_HFP_HF_STATE_ACTIVE,
    BTSTACK_HFP_HF_STATE_HELD,
    BTSTACK_HFP_HF_STATE_ACTIVE_WITH_INCOMING,
    BTSTACK_HFP_HF_STATE_ACTIVE_WITH_HELD,
    BTSTACK_HFP_HF_STATE_MULTIPARTY
} btstack_hfp_hf_state_t;

typedef enum {
    BTSTACK_HFP_HF_EVENT_DISCONNECTED = 0,
    BTSTACK_HFP_HF_EVENT_CONNECTING,
    BTSTACK_HFP_HF_EVENT_CONNECTED,
    BTSTACK_HFP_HF_EVENT_CALL_IDLE,
    BTSTACK_HFP_HF_EVENT_CALL_OUTGOING,
    BTSTACK_HFP_HF_EVENT_CALL_INCOMING,
    BTSTACK_HFP_HF_EVENT_CALL_ACTIVE,
    BTSTACK_HFP_HF_EVENT_CALL_HELD,
    BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_INCOMING,
    BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_HELD,
    BTSTACK_HFP_HF_EVENT_CALL_MULTIPARTY,
    BTSTACK_HFP_HF_EVENT_CONNECTION_LOST,
    BTSTACK_HFP_HF_EVENT_AUDIO,
    BTSTACK_HFP_HF_EVENT_VOICE_RECOGNITION,
    BTSTACK_HFP_HF_EVENT_SIGNAL,
    BTSTACK_HFP_HF_EVENT_NETWORK,
    BTSTACK_HFP_HF_EVENT_ROAM,
    BTSTACK_HFP_HF_EVENT_BATTERY,
    BTSTACK_HFP_HF_EVENT_NUMBER,
    BTSTACK_HFP_HF_EVENT_IN_BAND_RING,
    BTSTACK_HFP_HF_EVENT_RING,
    BTSTACK_HFP_HF_EVENT_MULTIPARTY
} btstack_hfp_hf_event_type_t;

typedef struct {
    btstack_hfp_hf_event_type_t type;
    btstack_hfp_audio_state_t audio_state;
    btstack_bd_addr_t address;
    uint32_t values[2];
    uint8_t value_count;
    char text[BTSTACK_HFP_HF_TEXT_MAX];
} btstack_hfp_hf_event_t;

typedef void (*btstack_hfp_hf_event_callback_t)(
    const btstack_hfp_hf_event_t *event);
typedef void (*btstack_hfp_audio_tx_ready_callback_t)(void);
typedef void (*btstack_hfp_direct_audio_callback_t)(
    const uint8_t *data,
    uint16_t length);

typedef struct {
    btstack_hfp_hf_event_callback_t event_callback;
    btstack_audio_sample_callback_t audio_sample_callback;
    const char *subscriber_number;
    const char *service_name;
    const char *manufacturer;
    uint32_t sdp_handle;
    uint32_t supported_features;
    uint8_t rfcomm_channel;
    uint8_t microphone_gain;
    uint8_t speaker_gain;
} btstack_hfp_hf_config_t;

/*
 * setup() initializes the native HFP HF and binds generic callbacks.
 * Call controls are implemented; SCO microphone/speaker routing remains
 * outside this control-only implementation.
 */
int btstack_hfp_hf_setup(
    const btstack_hfp_hf_config_t *config);
int btstack_hfp_hf_connect(const btstack_bd_addr_t address);
int btstack_hfp_hf_disconnect(void);
bool btstack_hfp_hf_is_connected(void);
bool btstack_hfp_hf_is_audio_connected(void);
btstack_hfp_hf_state_t btstack_hfp_hf_get_state(void);
const uint8_t *btstack_hfp_hf_get_peer_address(void);
int btstack_hfp_hf_set_event_callback(
    btstack_hfp_hf_event_callback_t callback);
int btstack_hfp_hf_set_audio_sample_callback(
    btstack_audio_sample_callback_t callback);
int btstack_hfp_hf_set_direct_audio_callback(
    btstack_hfp_direct_audio_callback_t callback);
int btstack_hfp_hf_set_audio_tx_ready_callback(
    btstack_hfp_audio_tx_ready_callback_t callback);
uint16_t btstack_hfp_hf_get_audio_tx_frame_length(void);
int btstack_hfp_hf_request_audio_can_send_now(void);
int btstack_hfp_hf_send_audio_frame(
    const uint8_t *data,
    uint16_t length);
int btstack_hfp_hf_control(
    btstack_hfp_hf_control_t control);
int btstack_hfp_hf_execute_control(
    uint32_t control,
    uint32_t value);
int btstack_hfp_hf_dial_number(const char *number);
int btstack_hfp_hf_redial(void);
int btstack_hfp_hf_send_dtmf(char code);
int btstack_hfp_hf_set_microphone_gain(uint8_t gain);
int btstack_hfp_hf_set_speaker_gain(uint8_t gain);

#ifdef __cplusplus
}
#endif

#endif
