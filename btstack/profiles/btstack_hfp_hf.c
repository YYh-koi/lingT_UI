#include "profiles/btstack_hfp_hf.h"

#include <string.h>

#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/hfp_hf.h"
#include "classic/sdp_server.h"
#include "hci.h"
#include "port/btstack_platform.h"

#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
#define BTSTACK_HFP_HF_SDP_RECORD_MAX 200u

static btstack_hfp_hf_config_t btstack_hfp_config;
static uint8_t btstack_hfp_sdp_record[BTSTACK_HFP_HF_SDP_RECORD_MAX];
static bool btstack_hfp_setup_done;
static bool btstack_hfp_connected;
static bool btstack_hfp_audio_connected;
static bool btstack_hfp_requested_disconnect;
static hci_con_handle_t btstack_hfp_acl_handle = HCI_CON_HANDLE_INVALID;
static btstack_bd_addr_t btstack_hfp_peer_address;
static btstack_hfp_hf_state_t btstack_hfp_state =
    BTSTACK_HFP_HF_STATE_IDLE;
static btstack_hfp_audio_state_t btstack_hfp_audio_state =
    BTSTACK_HFP_AUDIO_DISCONNECTED;
static uint8_t btstack_hfp_call;
static uint8_t btstack_hfp_callsetup;
static uint8_t btstack_hfp_callheld;
static char btstack_hfp_number[BTSTACK_HFP_HF_TEXT_MAX];
static hci_con_handle_t btstack_hfp_sco_handle = HCI_CON_HANDLE_INVALID;
static uint16_t btstack_hfp_tx_packet_length;
static btstack_hfp_audio_tx_ready_callback_t
    btstack_hfp_audio_tx_ready_callback;
static btstack_hfp_direct_audio_callback_t
    btstack_hfp_direct_audio_callback;
static btstack_packet_callback_registration_t
    btstack_hfp_hci_event_registration;
static bool btstack_hfp_audio_tx_waiting;

static void btstack_hfp_upstream_audio_handler(
    const uint8_t *data,
    uint16_t length)
{
    btstack_hfp_direct_audio_callback_t callback;

    btstack_platform_enter_critical();
    callback = btstack_hfp_direct_audio_callback;
    btstack_platform_exit_critical();
    if (callback != NULL) {
        callback(data, length);
    }
}

static void btstack_hfp_hci_event_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    btstack_hfp_audio_tx_ready_callback_t callback = NULL;
    bool waiting;

    (void)channel;
    (void)packet;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    btstack_platform_enter_critical();
    waiting = btstack_hfp_audio_tx_waiting;
    btstack_platform_exit_critical();
    if (!waiting || !hci_can_send_sco_packet_now()) {
        return;
    }
    btstack_platform_enter_critical();
    if (btstack_hfp_audio_tx_waiting) {
        btstack_hfp_audio_tx_waiting = false;
        callback = btstack_hfp_audio_tx_ready_callback;
    }
    btstack_platform_exit_critical();
    if (callback != NULL) {
        callback();
    }
}

static void btstack_hfp_audio_link_clear(void)
{
    btstack_platform_enter_critical();
    btstack_hfp_sco_handle = HCI_CON_HANDLE_INVALID;
    btstack_hfp_tx_packet_length = 0u;
    btstack_hfp_audio_tx_waiting = false;
    btstack_platform_exit_critical();
}

static int btstack_hfp_status(uint8_t status)
{
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
}

static void btstack_hfp_copy_text(
    char destination[BTSTACK_HFP_HF_TEXT_MAX],
    const char *source)
{
    uint16_t index = 0u;
    if (source != NULL) {
        while ((source[index] != '\0') &&
               (index < (BTSTACK_HFP_HF_TEXT_MAX - 1u))) {
            destination[index] = source[index];
            index++;
        }
    }
    destination[index] = '\0';
}

static void btstack_hfp_copy_bytes(
    char destination[BTSTACK_HFP_HF_TEXT_MAX],
    const uint8_t *source,
    uint8_t length)
{
    uint8_t index;
    if (length >= BTSTACK_HFP_HF_TEXT_MAX) {
        length = BTSTACK_HFP_HF_TEXT_MAX - 1u;
    }
    for (index = 0u; index < length; index++) {
        destination[index] = (char)source[index];
    }
    destination[length] = '\0';
}

static int btstack_hfp_text_equal(const char *left, const char *right)
{
    uint16_t index = 0u;
    if ((left == NULL) || (right == NULL)) {
        return 0;
    }
    while ((left[index] != '\0') && (right[index] != '\0')) {
        if (left[index] != right[index]) {
            return 0;
        }
        index++;
    }
    return left[index] == right[index];
}

static void btstack_hfp_emit(btstack_hfp_hf_event_t *event)
{
    if (btstack_hfp_config.event_callback != NULL) {
        btstack_hfp_config.event_callback(event);
    }
}

static void btstack_hfp_emit_value(
    btstack_hfp_hf_event_type_t type,
    uint32_t value)
{
    btstack_hfp_hf_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.values[0] = value;
    event.value_count = 1u;
    btstack_hfp_emit(&event);
}

static void btstack_hfp_emit_state_type(
    btstack_hfp_hf_event_type_t type)
{
    btstack_hfp_hf_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    memcpy(event.address, btstack_hfp_peer_address,
           sizeof(event.address));
    btstack_hfp_copy_text(event.text, btstack_hfp_number);
    btstack_hfp_emit(&event);
}

static void btstack_hfp_emit_state(void)
{
    btstack_hfp_hf_event_type_t type;
    switch (btstack_hfp_state) {
    case BTSTACK_HFP_HF_STATE_CONNECTING:
        type = BTSTACK_HFP_HF_EVENT_CONNECTING;
        break;
    case BTSTACK_HFP_HF_STATE_CONNECTED:
        type = BTSTACK_HFP_HF_EVENT_CONNECTED;
        break;
    case BTSTACK_HFP_HF_STATE_OUTGOING:
        type = BTSTACK_HFP_HF_EVENT_CALL_OUTGOING;
        break;
    case BTSTACK_HFP_HF_STATE_INCOMING:
        type = BTSTACK_HFP_HF_EVENT_CALL_INCOMING;
        break;
    case BTSTACK_HFP_HF_STATE_ACTIVE:
        type = BTSTACK_HFP_HF_EVENT_CALL_ACTIVE;
        break;
    case BTSTACK_HFP_HF_STATE_HELD:
        type = BTSTACK_HFP_HF_EVENT_CALL_HELD;
        break;
    case BTSTACK_HFP_HF_STATE_ACTIVE_WITH_INCOMING:
        type = BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_INCOMING;
        break;
    case BTSTACK_HFP_HF_STATE_ACTIVE_WITH_HELD:
        type = BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_HELD;
        break;
    case BTSTACK_HFP_HF_STATE_MULTIPARTY:
        type = BTSTACK_HFP_HF_EVENT_CALL_MULTIPARTY;
        break;
    case BTSTACK_HFP_HF_STATE_IDLE:
    default:
        type = BTSTACK_HFP_HF_EVENT_DISCONNECTED;
        break;
    }
    btstack_hfp_emit_state_type(type);
}

static void btstack_hfp_update_call_state(void)
{
    if ((btstack_hfp_callheld != 0u) && (btstack_hfp_callsetup == 2u)) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_MULTIPARTY;
    } else if ((btstack_hfp_call != 0u) &&
               (btstack_hfp_callsetup == 1u)) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_INCOMING;
    } else if ((btstack_hfp_call != 0u) &&
               (btstack_hfp_callheld != 0u)) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_HELD;
    } else if ((btstack_hfp_call == 0u) &&
               (btstack_hfp_callheld != 0u)) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_HELD;
    } else if ((btstack_hfp_callsetup == 2u) ||
               (btstack_hfp_callsetup == 3u)) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_OUTGOING;
    } else if (btstack_hfp_callsetup == 1u) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_INCOMING;
    } else if (btstack_hfp_call != 0u) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_ACTIVE;
    } else {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_CONNECTED;
    }
    if (btstack_hfp_state == BTSTACK_HFP_HF_STATE_CONNECTED) {
        btstack_hfp_emit_state_type(BTSTACK_HFP_HF_EVENT_CALL_IDLE);
    } else {
        btstack_hfp_emit_state();
    }
}

static void btstack_hfp_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    btstack_hfp_hf_event_t event;
    const char *indicator_name;
    uint8_t indicator_status;
    uint8_t subevent;

    (void)channel;
    (void)size;
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_HFP_META)) {
        return;
    }

    memset(&event, 0, sizeof(event));
    subevent = hci_event_hfp_meta_get_subevent_code(packet);
    switch (subevent) {
    case HFP_SUBEVENT_SERVICE_LEVEL_CONNECTION_ESTABLISHED:
        if (hfp_subevent_service_level_connection_established_get_status(
                packet) != ERROR_CODE_SUCCESS) {
            btstack_hfp_connected = false;
            btstack_hfp_acl_handle = HCI_CON_HANDLE_INVALID;
            btstack_hfp_state = BTSTACK_HFP_HF_STATE_IDLE;
            btstack_hfp_emit_state();
            return;
        }
        btstack_hfp_connected = true;
        btstack_hfp_acl_handle =
            hfp_subevent_service_level_connection_established_get_acl_handle(
                packet);
        hfp_subevent_service_level_connection_established_get_bd_addr(
            packet,
            btstack_hfp_peer_address);
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_CONNECTED;
        btstack_hfp_emit_state();
        (void)hfp_hf_enable_status_update_for_all_ag_indicators(
            btstack_hfp_acl_handle);
        (void)hfp_hf_activate_call_waiting_notification(
            btstack_hfp_acl_handle);
        (void)hfp_hf_activate_calling_line_notification(
            btstack_hfp_acl_handle);
        (void)hfp_hf_query_operator_selection(btstack_hfp_acl_handle);
        (void)hfp_hf_query_subscriber_number(btstack_hfp_acl_handle);
        (void)hfp_hf_set_speaker_gain(
            btstack_hfp_acl_handle,
            btstack_hfp_config.speaker_gain);
        return;

    case HFP_SUBEVENT_SERVICE_LEVEL_CONNECTION_RELEASED:
        btstack_hfp_connected = false;
        btstack_hfp_audio_connected = false;
        btstack_hfp_acl_handle = HCI_CON_HANDLE_INVALID;
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_IDLE;
        btstack_hfp_audio_state = BTSTACK_HFP_AUDIO_DISCONNECTED;
        btstack_hfp_audio_link_clear();
        if (btstack_hfp_requested_disconnect) {
            btstack_hfp_emit_state();
        } else {
            event.type = BTSTACK_HFP_HF_EVENT_CONNECTION_LOST;
            memcpy(event.address, btstack_hfp_peer_address,
                   sizeof(event.address));
            btstack_hfp_emit(&event);
        }
        btstack_hfp_requested_disconnect = false;
        return;

    case HFP_SUBEVENT_AUDIO_CONNECTION_ESTABLISHED:
        btstack_hfp_audio_connected =
            hfp_subevent_audio_connection_established_get_status(packet) ==
                ERROR_CODE_SUCCESS;
        btstack_hfp_audio_state = btstack_hfp_audio_connected ?
            BTSTACK_HFP_AUDIO_CONNECTED :
            BTSTACK_HFP_AUDIO_DISCONNECTED;
        btstack_platform_enter_critical();
        if (btstack_hfp_audio_connected) {
            btstack_hfp_sco_handle =
                hfp_subevent_audio_connection_established_get_sco_handle(
                    packet);
            btstack_hfp_tx_packet_length =
                hfp_subevent_audio_connection_established_get_tx_packet_length(
                    packet);
        } else {
            btstack_hfp_sco_handle = HCI_CON_HANDLE_INVALID;
            btstack_hfp_tx_packet_length = 0u;
        }
        btstack_platform_exit_critical();
        event.type = BTSTACK_HFP_HF_EVENT_AUDIO;
        event.audio_state = btstack_hfp_audio_state;
        event.values[0] = btstack_hfp_audio_connected ? 1u : 0u;
        event.value_count = 1u;
        btstack_hfp_emit(&event);
        return;

    case HFP_SUBEVENT_AUDIO_CONNECTION_RELEASED:
        btstack_hfp_audio_connected = false;
        btstack_hfp_audio_state = BTSTACK_HFP_AUDIO_DISCONNECTED;
        btstack_hfp_audio_link_clear();
        event.type = BTSTACK_HFP_HF_EVENT_AUDIO;
        event.audio_state = btstack_hfp_audio_state;
        event.value_count = 1u;
        btstack_hfp_emit(&event);
        return;

    case HFP_SUBEVENT_AG_INDICATOR_STATUS_CHANGED:
        indicator_name =
            hfp_subevent_ag_indicator_status_changed_get_indicator_name(
                packet);
        indicator_status =
            hfp_subevent_ag_indicator_status_changed_get_indicator_status(
                packet);
        if (btstack_hfp_text_equal(indicator_name, "call")) {
            btstack_hfp_call = indicator_status;
        } else if (btstack_hfp_text_equal(indicator_name, "callsetup")) {
            btstack_hfp_callsetup = indicator_status;
        } else if (btstack_hfp_text_equal(indicator_name, "callheld")) {
            btstack_hfp_callheld = indicator_status;
        } else if (btstack_hfp_text_equal(indicator_name, "signal")) {
            btstack_hfp_emit_value(
                BTSTACK_HFP_HF_EVENT_SIGNAL,
                indicator_status);
            return;
        } else if (btstack_hfp_text_equal(indicator_name, "service")) {
            btstack_hfp_emit_value(
                BTSTACK_HFP_HF_EVENT_NETWORK,
                indicator_status);
            return;
        } else if (btstack_hfp_text_equal(indicator_name, "roam")) {
            btstack_hfp_emit_value(
                BTSTACK_HFP_HF_EVENT_ROAM,
                indicator_status);
            return;
        } else if (btstack_hfp_text_equal(indicator_name, "battchg")) {
            btstack_hfp_emit_value(
                BTSTACK_HFP_HF_EVENT_BATTERY,
                indicator_status);
            return;
        } else {
            return;
        }
        btstack_hfp_update_call_state();
        return;

    case HFP_SUBEVENT_CALLING_LINE_IDENTIFICATION_NOTIFICATION:
        btstack_hfp_copy_bytes(
            btstack_hfp_number,
            hfp_subevent_calling_line_identification_notification_get_number(
                packet),
            hfp_subevent_calling_line_identification_notification_get_number_length(
                packet));
        return;

    case HFP_SUBEVENT_CALL_WAITING_NOTIFICATION:
        btstack_hfp_copy_bytes(
            btstack_hfp_number,
            hfp_subevent_call_waiting_notification_get_number(packet),
            hfp_subevent_call_waiting_notification_get_number_length(packet));
        return;

    case HFP_SUBEVENT_RING:
    case HFP_SUBEVENT_START_RINGING:
        btstack_hfp_emit_value(BTSTACK_HFP_HF_EVENT_RING, 1u);
        return;

    case HFP_SUBEVENT_STOP_RINGING:
        btstack_hfp_emit_value(BTSTACK_HFP_HF_EVENT_RING, 0u);
        return;

    case HFP_SUBEVENT_NETWORK_OPERATOR_CHANGED:
        event.type = BTSTACK_HFP_HF_EVENT_NETWORK;
        btstack_hfp_copy_text(
            event.text,
            hfp_subevent_network_operator_changed_get_network_operator_name(
                packet));
        btstack_hfp_emit(&event);
        return;

    case HFP_SUBEVENT_SUBSCRIBER_NUMBER_INFORMATION:
        event.type = BTSTACK_HFP_HF_EVENT_NUMBER;
        btstack_hfp_copy_text(
            event.text,
            hfp_subevent_subscriber_number_information_get_bnip_number(
                packet));
        btstack_hfp_emit(&event);
        return;

    case HFP_SUBEVENT_IN_BAND_RING_TONE:
        btstack_hfp_emit_value(
            BTSTACK_HFP_HF_EVENT_IN_BAND_RING,
            hfp_subevent_in_band_ring_tone_get_status(packet));
        return;

    case HFP_SUBEVENT_VOICE_RECOGNITION_ACTIVATED:
        btstack_hfp_emit_value(
            BTSTACK_HFP_HF_EVENT_VOICE_RECOGNITION,
            hfp_subevent_voice_recognition_activated_get_status(packet) ==
                ERROR_CODE_SUCCESS ? 1u : 0u);
        return;

    case HFP_SUBEVENT_VOICE_RECOGNITION_DEACTIVATED:
        btstack_hfp_emit_value(
            BTSTACK_HFP_HF_EVENT_VOICE_RECOGNITION,
            0u);
        return;

    case HFP_SUBEVENT_ENHANCED_CALL_STATUS:
        event.type = BTSTACK_HFP_HF_EVENT_MULTIPARTY;
        event.values[0] =
            hfp_subevent_enhanced_call_status_get_clcc_idx(packet);
        event.values[1] =
            hfp_subevent_enhanced_call_status_get_clcc_status(packet);
        event.value_count = 2u;
        btstack_hfp_copy_text(
            event.text,
            hfp_subevent_enhanced_call_status_get_bnip_number(packet));
        btstack_hfp_emit(&event);
        return;

    case HFP_SUBEVENT_SPEAKER_VOLUME:
        btstack_hfp_config.speaker_gain =
            hfp_subevent_speaker_volume_get_gain(packet);
        return;

    default:
        return;
    }
}
#endif

int btstack_hfp_hf_setup(const btstack_hfp_hf_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    static const uint8_t codecs[] = { HFP_CODEC_CVSD/* , HFP_CODEC_MSBC */ };
    uint32_t features;
    uint8_t status;

    if ((config == NULL) || (config->rfcomm_channel == 0u) ||
        (config->service_name == NULL)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    btstack_hfp_config = *config;
    memset(btstack_hfp_peer_address, 0, sizeof(btstack_hfp_peer_address));
    memset(btstack_hfp_number, 0, sizeof(btstack_hfp_number));
    btstack_hfp_connected = false;
    btstack_hfp_audio_connected = false;
    btstack_hfp_requested_disconnect = false;
    btstack_hfp_acl_handle = HCI_CON_HANDLE_INVALID;
    btstack_hfp_audio_link_clear();
    btstack_hfp_state = BTSTACK_HFP_HF_STATE_IDLE;
    btstack_hfp_audio_state = BTSTACK_HFP_AUDIO_DISCONNECTED;
    btstack_hfp_call = 0u;
    btstack_hfp_callsetup = 0u;
    btstack_hfp_callheld = 0u;

    status = hfp_hf_init(config->rfcomm_channel);   //控制通道：RFCOMM 服务端
    if (status != ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_ERROR;
    }
    features = config->supported_features;
    if (features == 0u) {
        features =
            (1u << HFP_HFSF_CLI_PRESENTATION_CAPABILITY) |
            (1u << HFP_HFSF_REMOTE_VOLUME_CONTROL); /* |
            (1u << HFP_HFSF_CODEC_NEGOTIATION); */
    }
    hfp_hf_init_supported_features(features);
    hfp_hf_init_codecs(sizeof(codecs), codecs);
    hfp_hf_register_packet_handler(btstack_hfp_packet_handler);  //event 
    /*
     * BTstack 1.6.2 delivers its synthetic SCO_CAN_SEND_NOW only through
     * the singleton SCO handler. Keep that singleton owned by upstream HFP
     * for downlink audio, and use stack-context availability checks plus
     * ordinary HCI progress events for uplink readiness.
     */
    hfp_hf_set_direct_audio_callback(btstack_hfp_upstream_audio_handler);  //音频
    memset(&btstack_hfp_hci_event_registration, 0,
        sizeof(btstack_hfp_hci_event_registration));
    btstack_hfp_hci_event_registration.callback =
        btstack_hfp_hci_event_handler;
    hci_add_event_handler(&btstack_hfp_hci_event_registration);  //上行 SCO 发送时机探测
    (void)hfp_hf_set_default_microphone_gain(config->microphone_gain);
    (void)hfp_hf_set_default_speaker_gain(config->speaker_gain);
    hfp_hf_create_sdp_record(
        btstack_hfp_sdp_record,
        config->sdp_handle,
        config->rfcomm_channel,
        config->service_name,
        features,
        0);
    status = sdp_register_service(btstack_hfp_sdp_record);
    if (status != ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_ERROR;
    }
    btstack_event_register_hfp_audio_sample_callback(
        config->audio_sample_callback);
    btstack_hfp_setup_done = true;
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_connect(const btstack_bd_addr_t address)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    int status;

    if (!btstack_hfp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (address == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memcpy(btstack_hfp_peer_address, address, sizeof(btstack_hfp_peer_address));
    btstack_hfp_state = BTSTACK_HFP_HF_STATE_CONNECTING;
    btstack_hfp_emit_state();
    status = btstack_hfp_status(
        hfp_hf_establish_service_level_connection(
            btstack_hfp_peer_address));
    if (status != BTSTACK_STATUS_OK) {
        btstack_hfp_state = BTSTACK_HFP_HF_STATE_IDLE;
        btstack_hfp_emit_state();
    }
    return status;
#else
    (void)address;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_disconnect(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (!btstack_hfp_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_hfp_requested_disconnect = true;
    return btstack_hfp_status(
        hfp_hf_release_service_level_connection(btstack_hfp_acl_handle));
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

bool btstack_hfp_hf_is_connected(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    return btstack_hfp_connected;
#else
    return false;
#endif
}

bool btstack_hfp_hf_is_audio_connected(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    return btstack_hfp_audio_connected;
#else
    return false;
#endif
}

btstack_hfp_hf_state_t btstack_hfp_hf_get_state(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    return btstack_hfp_state;
#else
    return BTSTACK_HFP_HF_STATE_IDLE;
#endif
}

const uint8_t *btstack_hfp_hf_get_peer_address(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    return btstack_hfp_peer_address;
#else
    return NULL;
#endif
}

int btstack_hfp_hf_set_event_callback(
    btstack_hfp_hf_event_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (!btstack_hfp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_hfp_config.event_callback = callback;
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_set_audio_sample_callback(
    btstack_audio_sample_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (!btstack_hfp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_hfp_config.audio_sample_callback = callback;
    btstack_event_register_hfp_audio_sample_callback(callback);
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_set_direct_audio_callback(
    btstack_hfp_direct_audio_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    btstack_platform_enter_critical();
    btstack_hfp_direct_audio_callback = callback;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_set_audio_tx_ready_callback(
    btstack_hfp_audio_tx_ready_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    btstack_platform_enter_critical();
    btstack_hfp_audio_tx_ready_callback = callback;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

uint16_t btstack_hfp_hf_get_audio_tx_frame_length(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    uint16_t length;
    btstack_platform_enter_critical();
    length = btstack_hfp_tx_packet_length;
    btstack_platform_exit_critical();
    return length;
#else
    return 0u;
#endif
}

int btstack_hfp_hf_request_audio_can_send_now(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    bool connected;
    btstack_hfp_audio_tx_ready_callback_t callback = NULL;
    btstack_platform_enter_critical();
    connected = (btstack_hfp_sco_handle != HCI_CON_HANDLE_INVALID) &&
        (btstack_hfp_tx_packet_length != 0u);
    btstack_platform_exit_critical();
    if (!connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (hci_can_send_sco_packet_now()) {
        btstack_platform_enter_critical();
        btstack_hfp_audio_tx_waiting = false;
        callback = btstack_hfp_audio_tx_ready_callback;
        btstack_platform_exit_critical();
        if (callback != NULL) {
            callback();
        }
    } else {
        btstack_platform_enter_critical();
        btstack_hfp_audio_tx_waiting = true;
        btstack_platform_exit_critical();
    }
    return BTSTACK_STATUS_OK;
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_send_audio_frame(
    const uint8_t *data,
    uint16_t length)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    hci_con_handle_t sco_handle;
    uint16_t frame_length;
    uint8_t *packet;
    uint8_t status;

    if ((data == NULL) || (length == 0u) || (length > 255u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    btstack_platform_enter_critical();
    sco_handle = btstack_hfp_sco_handle;
    frame_length = btstack_hfp_tx_packet_length;
    btstack_platform_exit_critical();
    if ((sco_handle == HCI_CON_HANDLE_INVALID) ||
        (length != frame_length) || !hci_can_send_sco_packet_now()) {
        return BTSTACK_STATUS_NOT_READY;
    }
    hci_reserve_packet_buffer();
    packet = hci_get_outgoing_packet_buffer();
    packet[0] = (uint8_t)(sco_handle & 0xffu);
    packet[1] = (uint8_t)((sco_handle >> 8) & 0x0fu);
    packet[2] = (uint8_t)length;
    memcpy(&packet[HCI_SCO_HEADER_SIZE], data, length);
    status = hci_send_sco_packet_buffer(
        (int)(HCI_SCO_HEADER_SIZE + length));
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
#else
    (void)data;
    (void)length;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_control(btstack_hfp_hf_control_t control)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (!btstack_hfp_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    switch (control) {
    case BTSTACK_HFP_HF_CONTROL_ANSWER:
        return btstack_hfp_status(
            hfp_hf_answer_incoming_call(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_HANG_UP:
        return btstack_hfp_status(
            hfp_hf_terminate_call(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_REDIAL:
        return btstack_hfp_status(
            hfp_hf_redial_last_number(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_RELEASE_HELD:
        return btstack_hfp_status(
            hfp_hf_user_busy(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_HOLD_ACTIVE_ACCEPT_OTHER:
        return btstack_hfp_status(
            hfp_hf_end_active_and_accept_other(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_JOIN_CALLS:
        return btstack_hfp_status(
            hfp_hf_swap_calls(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_CONNECT_AUDIO:
        return btstack_hfp_status(
            hfp_hf_establish_audio_connection(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_DISCONNECT_AUDIO:
        return btstack_hfp_status(
            hfp_hf_release_audio_connection(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_ENABLE_VOICE_RECOGNITION:
        return btstack_hfp_status(
            hfp_hf_activate_voice_recognition(btstack_hfp_acl_handle));
    case BTSTACK_HFP_HF_CONTROL_DISABLE_VOICE_RECOGNITION:
        return btstack_hfp_status(
            hfp_hf_deactivate_voice_recognition(btstack_hfp_acl_handle));
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
#else
    (void)control;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_execute_control(
    uint32_t control,
    uint32_t value)
{
    enum {
        HFP_REQUEST_ANSWER = 0,
        HFP_REQUEST_HANG_UP,
        HFP_REQUEST_MULTIPARTY,
        HFP_REQUEST_TOGGLE_AUDIO,
        HFP_REQUEST_SET_AUDIO,
        HFP_REQUEST_VOICE_RECOGNITION
    };

    switch (control) {
    case HFP_REQUEST_ANSWER:
        return btstack_hfp_hf_control(BTSTACK_HFP_HF_CONTROL_ANSWER);
    case HFP_REQUEST_HANG_UP:
        return btstack_hfp_hf_control(BTSTACK_HFP_HF_CONTROL_HANG_UP);
    case HFP_REQUEST_MULTIPARTY:
        if (value == 0u) {
            return btstack_hfp_hf_control(
                BTSTACK_HFP_HF_CONTROL_RELEASE_HELD);
        }
        if (value == 1u) {
            return btstack_hfp_hf_control(
                BTSTACK_HFP_HF_CONTROL_HOLD_ACTIVE_ACCEPT_OTHER);
        }
        return btstack_hfp_hf_control(
            BTSTACK_HFP_HF_CONTROL_JOIN_CALLS);
    case HFP_REQUEST_TOGGLE_AUDIO:
        return btstack_hfp_hf_control(
            btstack_hfp_hf_is_audio_connected() ?
                BTSTACK_HFP_HF_CONTROL_DISCONNECT_AUDIO :
                BTSTACK_HFP_HF_CONTROL_CONNECT_AUDIO);
    case HFP_REQUEST_SET_AUDIO:
        return btstack_hfp_hf_control(
            value ? BTSTACK_HFP_HF_CONTROL_CONNECT_AUDIO :
                BTSTACK_HFP_HF_CONTROL_DISCONNECT_AUDIO);
    case HFP_REQUEST_VOICE_RECOGNITION:
        return btstack_hfp_hf_control(
            value ?
                BTSTACK_HFP_HF_CONTROL_ENABLE_VOICE_RECOGNITION :
                BTSTACK_HFP_HF_CONTROL_DISABLE_VOICE_RECOGNITION);
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
}

int btstack_hfp_hf_dial_number(const char *number)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if ((number == NULL) || (number[0] == '\0')) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_hfp_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_hfp_status(
        hfp_hf_dial_number(btstack_hfp_acl_handle, (char *)number));
#else
    (void)number;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_redial(void)
{
    return btstack_hfp_hf_control(BTSTACK_HFP_HF_CONTROL_REDIAL);
}

int btstack_hfp_hf_send_dtmf(char code)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (!btstack_hfp_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_hfp_status(
        hfp_hf_send_dtmf_code(btstack_hfp_acl_handle, code));
#else
    (void)code;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_set_microphone_gain(uint8_t gain)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (gain > 15u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    btstack_hfp_config.microphone_gain = gain;
    if (!btstack_hfp_connected) {
        return btstack_hfp_status(hfp_hf_set_default_microphone_gain(gain));
    }
    return btstack_hfp_status(
        hfp_hf_set_microphone_gain(btstack_hfp_acl_handle, gain));
#else
    (void)gain;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hfp_hf_set_speaker_gain(uint8_t gain)
{
#if BTSTACK_CLASSIC && BTSTACK_HFP_HF
    if (gain > 15u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    btstack_hfp_config.speaker_gain = gain;
    if (!btstack_hfp_connected) {
        return btstack_hfp_status(hfp_hf_set_default_speaker_gain(gain));
    }
    return btstack_hfp_status(
        hfp_hf_set_speaker_gain(btstack_hfp_acl_handle, gain));
#else
    (void)gain;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}
