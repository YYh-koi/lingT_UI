#include "profiles/btstack_a2dp_sink.h"

#include <string.h>

#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/a2dp_sink.h"
#include "classic/avdtp.h"
#include "classic/sdp_server.h"
#include "btstack_a2dp_media.h"

#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
#define BTSTACK_A2DP_SDP_RECORD_MAX 200u

enum {
    BTSTACK_A2DP_PCM_CHANNEL_COUNT = 2
};

static btstack_a2dp_sink_config_t btstack_a2dp_config;
static uint8_t btstack_a2dp_sdp_record[BTSTACK_A2DP_SDP_RECORD_MAX];
static uint8_t btstack_a2dp_sbc_codec_configuration[4];
static uint16_t btstack_a2dp_cid;
static btstack_bd_addr_t btstack_a2dp_peer_address;
static btstack_a2dp_sink_state_t btstack_a2dp_state =
    BTSTACK_A2DP_SINK_STATE_IDLE;
static bool btstack_a2dp_setup_done;
static bool btstack_a2dp_requested_disconnect;

static const uint8_t btstack_a2dp_sbc_codec_capabilities[] = {
    (uint8_t)(((AVDTP_SBC_48000 | AVDTP_SBC_44100) << 4) |
              AVDTP_SBC_JOINT_STEREO | AVDTP_SBC_STEREO |
              AVDTP_SBC_DUAL_CHANNEL | AVDTP_SBC_MONO),
    (uint8_t)(((AVDTP_SBC_BLOCK_LENGTH_16 |
                AVDTP_SBC_BLOCK_LENGTH_12 |
                AVDTP_SBC_BLOCK_LENGTH_8 |
                AVDTP_SBC_BLOCK_LENGTH_4) << 4) |
              ((AVDTP_SBC_SUBBANDS_8 |
                AVDTP_SBC_SUBBANDS_4) << 2) |
              AVDTP_SBC_ALLOCATION_METHOD_LOUDNESS |
              AVDTP_SBC_ALLOCATION_METHOD_SNR),
    2u,
    53u
};

static void btstack_a2dp_emit(
    btstack_a2dp_sink_event_type_t type)
{
    btstack_a2dp_sink_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    memcpy(event.address, btstack_a2dp_peer_address,
           sizeof(event.address));
    if (btstack_a2dp_config.event_callback != NULL) {
        btstack_a2dp_config.event_callback(&event);
    }
}

static void btstack_a2dp_emit_pcm_config(
    uint32_t sample_rate,
    uint8_t channel_count)
{
    btstack_a2dp_sink_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG;
    memcpy(event.address, btstack_a2dp_peer_address,
           sizeof(event.address));
    event.sample_rate = sample_rate;
    event.channel_count = channel_count;
    event.bits_per_sample = 16u;
    if (btstack_a2dp_config.event_callback != NULL) {
        btstack_a2dp_config.event_callback(&event);
    }
}

static void btstack_a2dp_set_state_event(
    btstack_a2dp_sink_state_t state,
    btstack_a2dp_sink_event_type_t event_type)
{
    if (btstack_a2dp_state == state) {
        return;
    }
    btstack_a2dp_state = state;
    btstack_a2dp_emit(event_type);
}

static void btstack_a2dp_set_state(btstack_a2dp_sink_state_t state)
{
    static const btstack_a2dp_sink_event_type_t event_types[] = {
        BTSTACK_A2DP_SINK_EVENT_DISCONNECTED,
        BTSTACK_A2DP_SINK_EVENT_CONNECTING,
        BTSTACK_A2DP_SINK_EVENT_CONNECTED,
        BTSTACK_A2DP_SINK_EVENT_STREAM_OPEN,
        BTSTACK_A2DP_SINK_EVENT_STREAMING,
        BTSTACK_A2DP_SINK_EVENT_SUSPENDED
    };

    btstack_a2dp_set_state_event(
        state, event_types[(uint8_t)state]);
}

static void btstack_a2dp_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    uint8_t subevent;

    (void)channel;
    (void)size;
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_A2DP_META)) {
        return;
    }
    subevent = hci_event_a2dp_meta_get_subevent_code(packet);
    switch (subevent) {
    case A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION:
        {
        uint32_t sample_rate =
            a2dp_subevent_signaling_media_codec_sbc_configuration_get_sampling_frequency(
                packet);
        if (btstack_a2dp_media_configure(
                sample_rate, BTSTACK_A2DP_PCM_CHANNEL_COUNT) == 0) {
            btstack_a2dp_emit_pcm_config(
                sample_rate, BTSTACK_A2DP_PCM_CHANNEL_COUNT);
        }
        }
        return;

    case A2DP_SUBEVENT_SIGNALING_CONNECTION_ESTABLISHED:
        if (a2dp_subevent_signaling_connection_established_get_status(
                packet) != ERROR_CODE_SUCCESS) {
            btstack_a2dp_cid = 0u;
            btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_IDLE);
            return;
        }
        btstack_a2dp_cid =
            a2dp_subevent_signaling_connection_established_get_a2dp_cid(
                packet);
        a2dp_subevent_signaling_connection_established_get_bd_addr(
            packet,
            btstack_a2dp_peer_address);
        btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_CONNECTED);
        return;

    case A2DP_SUBEVENT_STREAM_ESTABLISHED:
        if (a2dp_subevent_stream_established_get_status(packet) ==
                ERROR_CODE_SUCCESS) {
            btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_STREAM_OPEN);
        }
        return;

#ifdef ENABLE_AVDTP_ACCEPTOR_EXPLICIT_START_STREAM_CONFIRMATION
    case A2DP_SUBEVENT_START_STREAM_REQUESTED:
        a2dp_sink_start_stream_accept(
            a2dp_subevent_start_stream_requested_get_a2dp_cid(packet),
            a2dp_subevent_start_stream_requested_get_local_seid(packet));
        return;
#endif

    case A2DP_SUBEVENT_STREAM_STARTED:
        btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_STREAMING);
        return;

    case A2DP_SUBEVENT_STREAM_SUSPENDED:
        btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_SUSPENDED);
        return;

    case A2DP_SUBEVENT_STREAM_RELEASED:
        btstack_a2dp_media_reset();
        btstack_a2dp_set_state_event(
            BTSTACK_A2DP_SINK_STATE_CONNECTED,
            BTSTACK_A2DP_SINK_EVENT_STREAM_CLOSED);
        return;

    case A2DP_SUBEVENT_SIGNALING_CONNECTION_RELEASED:
        btstack_a2dp_media_reset();
        btstack_a2dp_cid = 0u;
        if (btstack_a2dp_requested_disconnect) {
            btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_IDLE);
        } else if (btstack_a2dp_state != BTSTACK_A2DP_SINK_STATE_IDLE) {
            btstack_a2dp_state = BTSTACK_A2DP_SINK_STATE_IDLE;
            btstack_a2dp_emit(BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST);
        }
        btstack_a2dp_requested_disconnect = false;
        return;

    default:
        return;
    }
}

static void btstack_a2dp_audio_sample_dispatch(
    const btstack_audio_sample_t *sample)
{
    if (btstack_a2dp_config.audio_sample_callback != NULL) {
        btstack_a2dp_config.audio_sample_callback(sample);
    }
}

static void btstack_a2dp_media_data_dispatch(
    const btstack_media_packet_t *packet)
{
    if (btstack_a2dp_config.media_data_callback != NULL) {
        btstack_a2dp_config.media_data_callback(packet);
    }
}

static void btstack_a2dp_media_handler(
    uint8_t local_seid,
    uint8_t *packet,
    uint16_t size)
{
    btstack_media_packet_t media;

    (void)local_seid;
    if (btstack_a2dp_config.media_data_callback != NULL) {
        memset(&media, 0, sizeof(media));
        media.format = BTSTACK_AUDIO_FORMAT_SBC;
        media.data = packet;
        media.data_len = size;
        btstack_a2dp_config.media_data_callback(&media);
    }
    btstack_a2dp_media_process(packet, size);
}
#endif

int btstack_a2dp_sink_setup(const btstack_a2dp_sink_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    uint8_t status;

    if ((config == NULL) || (config->service_name == NULL)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    btstack_a2dp_config = *config;
    btstack_a2dp_cid = 0u;
    btstack_a2dp_requested_disconnect = false;
    btstack_a2dp_media_init(btstack_a2dp_audio_sample_dispatch);
    memset(btstack_a2dp_peer_address, 0,
           sizeof(btstack_a2dp_peer_address));

    a2dp_sink_init();
    a2dp_sink_register_packet_handler(btstack_a2dp_packet_handler);// ← 控制：信令/连接/配置
    a2dp_sink_register_media_handler(btstack_a2dp_media_handler);// ← 数据：AVDTP 媒体包
    if (a2dp_sink_create_stream_endpoint(
            AVDTP_AUDIO,
            AVDTP_CODEC_SBC,
            btstack_a2dp_sbc_codec_capabilities,
            sizeof(btstack_a2dp_sbc_codec_capabilities),
            btstack_a2dp_sbc_codec_configuration,
            sizeof(btstack_a2dp_sbc_codec_configuration)) == NULL) {
        return BTSTACK_STATUS_ERROR;
    }
    a2dp_sink_create_sdp_record(
        btstack_a2dp_sdp_record,
        config->sdp_handle,
        AVDTP_SINK_FEATURE_MASK_HEADPHONE,
        config->service_name,
        config->manufacturer);
    status = sdp_register_service(btstack_a2dp_sdp_record);
    if (status != ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_ERROR;
    }
    /* btstack_event_register_a2dp_audio_sample_callback(
        btstack_a2dp_audio_sample_dispatch); */
    btstack_event_register_a2dp_media_data_callback(
        btstack_a2dp_media_data_dispatch);
    btstack_a2dp_setup_done = true;
    btstack_a2dp_state = BTSTACK_A2DP_SINK_STATE_IDLE;
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_a2dp_sink_connect(const btstack_bd_addr_t address)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    uint8_t status;
    if (!btstack_a2dp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (address == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memcpy(btstack_a2dp_peer_address, address,
           sizeof(btstack_a2dp_peer_address));
    btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_CONNECTING);
    status = a2dp_sink_establish_stream(
        btstack_a2dp_peer_address,
        &btstack_a2dp_cid);
    if (status != ERROR_CODE_SUCCESS) {
        btstack_a2dp_cid = 0u;
        btstack_a2dp_set_state(BTSTACK_A2DP_SINK_STATE_IDLE);
    }
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
#else
    (void)address;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_a2dp_sink_disconnect(void)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    if ((btstack_a2dp_cid == 0u) ||
        (btstack_a2dp_state == BTSTACK_A2DP_SINK_STATE_IDLE)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_a2dp_requested_disconnect = true;
    a2dp_sink_disconnect(btstack_a2dp_cid);
    return BTSTACK_STATUS_OK;
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

bool btstack_a2dp_sink_is_connected(void)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    return (btstack_a2dp_state != BTSTACK_A2DP_SINK_STATE_IDLE) &&
           (btstack_a2dp_state != BTSTACK_A2DP_SINK_STATE_CONNECTING);
#else
    return false;
#endif
}

btstack_a2dp_sink_state_t btstack_a2dp_sink_get_state(void)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    return btstack_a2dp_state;
#else
    return BTSTACK_A2DP_SINK_STATE_IDLE;
#endif
}

const uint8_t *btstack_a2dp_sink_get_peer_address(void)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    return btstack_a2dp_peer_address;
#else
    return NULL;
#endif
}

int btstack_a2dp_sink_set_event_callback(
    btstack_a2dp_sink_event_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    if (!btstack_a2dp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_a2dp_config.event_callback = callback;
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_a2dp_sink_set_audio_sample_callback(
    btstack_audio_sample_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    if (!btstack_a2dp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_a2dp_config.audio_sample_callback = callback;
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_a2dp_sink_set_media_data_callback(
    btstack_media_data_callback_t callback)
{
#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK
    if (!btstack_a2dp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_a2dp_config.media_data_callback = callback;
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_a2dp_sink_control(btstack_a2dp_sink_control_t control)
{
    (void)control;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}
