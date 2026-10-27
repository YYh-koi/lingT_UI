#include "core/btstack_profile_manager.h"

#include <string.h>

#include "adapter/btstack_types.h"
#include "adapter/btstack_adapter.h"
#include "classic/rfcomm.h"
#include "classic/sdp_server.h"
#include "port/btstack_platform.h"
#include "profiles/btstack_a2dp_sink.h"
#include "profiles/btstack_avrcp.h"
#include "profiles/btstack_avrcp_cover_art.h"
#include "profiles/btstack_hfp_hf.h"
#include "profiles/btstack_gatt_client.h"
#include "profiles/btstack_gatt_server.h"
#include "profiles/btstack_hogp_device.h"
#include "profiles/btstack_pbap_client.h"
#include "btstack_core_internal.h"
#include "btstack_call_audio_tx.h"

typedef enum {
    BTSTACK_PROFILE_MANAGER_UNINITIALIZED = 0,
    BTSTACK_PROFILE_MANAGER_INITIALIZING,
    BTSTACK_PROFILE_MANAGER_READY,
    BTSTACK_PROFILE_MANAGER_FAILED
} btstack_profile_manager_lifecycle_t;

static btstack_profile_manager_lifecycle_t manager_lifecycle;
static btstack_profile_manager_config_t manager_config;
static btstack_profile_manager_classic_state_t manager_classic_state;
static btstack_profile_manager_le_state_t manager_le_state;
static bt_xstack_addr_t manager_profile_peers[3];
static uint8_t manager_profile_peer_valid[3];
static uint8_t manager_disconnect_requested[3];

static int btstack_profile_manager_profile_index(bt_xstack_profile_t profile)
{
    switch (profile) {
    case BT_XSTACK_PROFILE_HFP_HF: return 0;
    case BT_XSTACK_PROFILE_A2DP_SINK: return 1;
    case BT_XSTACK_PROFILE_AVRCP: return 2;
    default: return -1;
    }
}

#if BTSTACK_CLASSIC
static int btstack_profile_manager_address_is_zero(
    const bt_xstack_addr_t *address)
{
    uint8_t index;
    for (index = 0u; index < BT_XSTACK_ADDRESS_LEN; index++) {
        if (address->bytes[index] != 0u) return 0;
    }
    return 1;
}

static void btstack_profile_manager_resolve_peer(
    bt_xstack_profile_t profile,
    btstack_bd_addr_t native_address,
    bt_xstack_addr_t *address)
{
    memcpy(address->bytes, native_address, BT_XSTACK_ADDRESS_LEN);
    if (btstack_profile_manager_address_is_zero(address)) {
        (void)btstack_core_state_get_profile_peer(profile, address);
    }
    if (!btstack_profile_manager_address_is_zero(address)) {
        btstack_core_state_set_profile_peer(profile, address);
    }
}

static bt_xstack_connection_state_t
btstack_profile_manager_hfp_connection_state(
    btstack_hfp_hf_state_t state)
{
    if (state == BTSTACK_HFP_HF_STATE_CONNECTING) {
        return BT_XSTACK_CONNECTION_CONNECTING;
    }
    if (state >= BTSTACK_HFP_HF_STATE_CONNECTED) {
        return BT_XSTACK_CONNECTION_CONNECTED;
    }
    return BT_XSTACK_CONNECTION_DISCONNECTED;
}

static int btstack_profile_manager_hfp_event_state(
    btstack_hfp_hf_event_type_t type,
    btstack_hfp_hf_state_t *state)
{
    if (state == NULL) return 0;
    switch (type) {
    case BTSTACK_HFP_HF_EVENT_CONNECTING:
        *state = BTSTACK_HFP_HF_STATE_CONNECTING; return 1;
    case BTSTACK_HFP_HF_EVENT_CONNECTED:
    case BTSTACK_HFP_HF_EVENT_CALL_IDLE:
        *state = BTSTACK_HFP_HF_STATE_CONNECTED; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_OUTGOING:
        *state = BTSTACK_HFP_HF_STATE_OUTGOING; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_INCOMING:
        *state = BTSTACK_HFP_HF_STATE_INCOMING; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE:
        *state = BTSTACK_HFP_HF_STATE_ACTIVE; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_HELD:
        *state = BTSTACK_HFP_HF_STATE_HELD; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_INCOMING:
        *state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_INCOMING; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_HELD:
        *state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_HELD; return 1;
    case BTSTACK_HFP_HF_EVENT_CALL_MULTIPARTY:
        *state = BTSTACK_HFP_HF_STATE_MULTIPARTY; return 1;
    case BTSTACK_HFP_HF_EVENT_CONNECTION_LOST:
    case BTSTACK_HFP_HF_EVENT_DISCONNECTED:
        *state = BTSTACK_HFP_HF_STATE_IDLE; return 1;
    default:
        return 0;
    }
}

static uint8_t btstack_profile_manager_a2dp_state_value(
    btstack_a2dp_sink_state_t state)
{
    switch (state) {
    case BTSTACK_A2DP_SINK_STATE_IDLE: return 1u;
    case BTSTACK_A2DP_SINK_STATE_CONNECTING: return 2u;
    case BTSTACK_A2DP_SINK_STATE_CONNECTED: return 3u;
    case BTSTACK_A2DP_SINK_STATE_STREAM_OPEN:
    case BTSTACK_A2DP_SINK_STATE_SUSPENDED: return 4u;
    case BTSTACK_A2DP_SINK_STATE_STREAMING: return 5u;
    default: return 1u;
    }
}

static int btstack_profile_manager_a2dp_event_state(
    btstack_a2dp_sink_event_type_t type,
    btstack_a2dp_sink_state_t *state)
{
    if (state == NULL) return 0;
    switch (type) {
    case BTSTACK_A2DP_SINK_EVENT_CONNECTING:
        *state = BTSTACK_A2DP_SINK_STATE_CONNECTING; return 1;
    case BTSTACK_A2DP_SINK_EVENT_CONNECTED:
    case BTSTACK_A2DP_SINK_EVENT_STREAM_CLOSED:
        *state = BTSTACK_A2DP_SINK_STATE_CONNECTED; return 1;
    case BTSTACK_A2DP_SINK_EVENT_STREAM_OPEN:
        *state = BTSTACK_A2DP_SINK_STATE_STREAM_OPEN; return 1;
    case BTSTACK_A2DP_SINK_EVENT_STREAMING:
        *state = BTSTACK_A2DP_SINK_STATE_STREAMING; return 1;
    case BTSTACK_A2DP_SINK_EVENT_SUSPENDED:
        *state = BTSTACK_A2DP_SINK_STATE_SUSPENDED; return 1;
    case BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST:
    case BTSTACK_A2DP_SINK_EVENT_DISCONNECTED:
        *state = BTSTACK_A2DP_SINK_STATE_IDLE; return 1;
    default:
        return 0;
    }
}

static uint8_t btstack_profile_manager_avrcp_state_value(
    btstack_avrcp_state_t state)
{
    if (state == BTSTACK_AVRCP_STATE_CONNECTING) return 2u;
    if (state == BTSTACK_AVRCP_STATE_CONNECTED) return 3u;
    return 1u;
}

static int btstack_profile_manager_avrcp_event_state(
    btstack_avrcp_event_type_t type,
    btstack_avrcp_state_t *state)
{
    if (state == NULL) return 0;
    switch (type) {
    case BTSTACK_AVRCP_EVENT_CONNECTING:
        *state = BTSTACK_AVRCP_STATE_CONNECTING; return 1;
    case BTSTACK_AVRCP_EVENT_CONNECTED:
        *state = BTSTACK_AVRCP_STATE_CONNECTED; return 1;
    case BTSTACK_AVRCP_EVENT_DISCONNECTED:
        *state = BTSTACK_AVRCP_STATE_IDLE; return 1;
    default:
        return 0;
    }
}

static int btstack_profile_manager_pbap_event_state(
    btstack_pbap_client_event_type_t type,
    btstack_pbap_client_state_t *state)
{
    if (state == NULL) return 0;
    switch (type) {
    case BTSTACK_PBAP_CLIENT_EVENT_CONNECTING:
        *state = BTSTACK_PBAP_CLIENT_STATE_CONNECTING; return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATING:
        *state = BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING; return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_PULLING:
        *state = BTSTACK_PBAP_CLIENT_STATE_PULLING; return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_PAUSED:
        *state = BTSTACK_PBAP_CLIENT_STATE_PAUSED; return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTING:
        *state = BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING; return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED:
        *state = BTSTACK_PBAP_CLIENT_STATE_IDLE; return 1;
    default:
        return 0;
    }
}

static void btstack_profile_manager_hfp_event(
    const btstack_hfp_hf_event_t *event)
{
    btstack_hfp_hf_event_t mapped;
    bt_xstack_addr_t address;
    btstack_core_hfp_snapshot_t snapshot;
    btstack_hfp_hf_state_t state;
    uint8_t previous_state;
    const uint8_t *native_peer;

    if (event == NULL) return;
    mapped = *event;
    btstack_profile_manager_resolve_peer(
        BT_XSTACK_PROFILE_HFP_HF, mapped.address, &address);
    if (btstack_profile_manager_address_is_zero(&address)) {
        native_peer = btstack_hfp_hf_get_peer_address();
        if (native_peer != NULL) {
            memcpy(address.bytes, native_peer, BT_XSTACK_ADDRESS_LEN);
            memcpy(mapped.address, native_peer, BT_XSTACK_ADDRESS_LEN);
            btstack_core_state_set_profile_peer(
                BT_XSTACK_PROFILE_HFP_HF, &address);
        }
    } else {
        memcpy(mapped.address, address.bytes, BT_XSTACK_ADDRESS_LEN);
    }
    (void)btstack_core_state_read_hfp(&snapshot);

    if (btstack_profile_manager_hfp_event_state(event->type, &state)) {
        btstack_platform_enter_critical();
        previous_state = manager_classic_state.hfp_state;
        manager_classic_state.hfp_state = (uint8_t)state;
        btstack_platform_exit_critical();
        if (state == BTSTACK_HFP_HF_STATE_IDLE) {
            btstack_call_audio_tx_session_changed(false, 0u);
        }
        if ((state != BTSTACK_HFP_HF_STATE_IDLE) ||
            btstack_core_state_take_disconnect_request(
                BT_XSTACK_PROFILE_HFP_HF) ||
            (previous_state == BT_XSTACK_HFP_STATE_CONNECTING) ||
            (event->type == BTSTACK_HFP_HF_EVENT_CONNECTION_LOST)) {
            btstack_core_event_mapper_post_connection(
                BT_XSTACK_PROFILE_HFP_HF,
                btstack_profile_manager_hfp_connection_state(state),
                &address,
                event->type == BTSTACK_HFP_HF_EVENT_CONNECTION_LOST);
        }
    }

    switch (event->type) {
    case BTSTACK_HFP_HF_EVENT_AUDIO:
        btstack_platform_enter_critical();
        manager_classic_state.hfp_audio_state =
            (event->audio_state == BTSTACK_HFP_AUDIO_CONNECTED) ? 1u : 0u;
        btstack_platform_exit_critical();
        if (event->audio_state == BTSTACK_HFP_AUDIO_CONNECTED) {
            btstack_call_audio_tx_session_changed(
                true, btstack_hfp_hf_get_audio_tx_frame_length());
        } else {
            btstack_call_audio_tx_session_changed(false, 0u);
        }
        break;
    case BTSTACK_HFP_HF_EVENT_VOICE_RECOGNITION:
        btstack_platform_enter_critical();
        manager_classic_state.hfp_voice_recognition =
            (uint8_t)event->values[0];
        btstack_platform_exit_critical();
        break;
    case BTSTACK_HFP_HF_EVENT_RING:
        if ((event->values[0] != 0u) &&
            ((snapshot.config & 2u) != 0u)) {
            (void)btstack_hfp_hf_control(
                BTSTACK_HFP_HF_CONTROL_ANSWER);
        }
        break;
    default:
        break;
    }
    btstack_core_event_mapper_hfp_event(&mapped);
}

static uint8_t btstack_profile_manager_pbap_event(
    const btstack_pbap_client_event_t *event)
{
    btstack_pbap_client_state_t state;
    uint8_t accepted;
    if (event == NULL) return 1u;
    if (btstack_profile_manager_pbap_event_state(event->type, &state)) {
        btstack_platform_enter_critical();
        manager_classic_state.pbap_state = (uint8_t)state;
        btstack_platform_exit_critical();
        return 1u;
    }
    btstack_platform_enter_critical();
    manager_classic_state.pbap_state =
        (uint8_t)btstack_pbap_client_get_state();
    btstack_platform_exit_critical();
    accepted = btstack_core_event_mapper_pbap_event(event);
    if (!accepted &&
        ((event->type == BTSTACK_PBAP_CLIENT_EVENT_CONTACT) ||
         (event->type == BTSTACK_PBAP_CLIENT_EVENT_CALL))) {
        btstack_platform_enter_critical();
        manager_classic_state.pbap_state =
            (uint8_t)BT_XSTACK_PHONEBOOK_STATE_PAUSED;
        btstack_platform_exit_critical();
    }
    return accepted;
}

static void btstack_profile_manager_a2dp_event(
    const btstack_a2dp_sink_event_t *event)
{
    btstack_a2dp_sink_event_t mapped;
    bt_xstack_addr_t address;
    btstack_a2dp_sink_state_t state;
    uint8_t value;
    uint8_t previous_state;

    if (event == NULL) return;
    mapped = *event;
    btstack_profile_manager_resolve_peer(
        BT_XSTACK_PROFILE_A2DP_SINK, mapped.address, &address);
    memcpy(mapped.address, address.bytes, BT_XSTACK_ADDRESS_LEN);

    if (btstack_profile_manager_a2dp_event_state(event->type, &state)) {
        value = btstack_profile_manager_a2dp_state_value(state);
        btstack_platform_enter_critical();
        previous_state = manager_classic_state.a2dp_state;
        manager_classic_state.a2dp_state = value;
        btstack_platform_exit_critical();
        if ((value != BT_XSTACK_A2DP_STATE_IDLE) ||
            btstack_core_state_take_disconnect_request(
                BT_XSTACK_PROFILE_A2DP_SINK) ||
            (previous_state == BT_XSTACK_A2DP_STATE_CONNECTING) ||
            (event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST)) {
            btstack_core_event_mapper_post_connection(
                BT_XSTACK_PROFILE_A2DP_SINK,
                (value == 2u) ? BT_XSTACK_CONNECTION_CONNECTING :
                    ((value >= 3u) ? BT_XSTACK_CONNECTION_CONNECTED :
                        BT_XSTACK_CONNECTION_DISCONNECTED),
                &address,
                event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST);
        }
        if ((event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTED) &&
            !btstack_avrcp_is_connected() &&
            (btstack_avrcp_get_state() == BTSTACK_AVRCP_STATE_IDLE)) {
            btstack_core_state_set_profile_peer(
                BT_XSTACK_PROFILE_AVRCP, &address);
            (void)btstack_avrcp_connect(mapped.address);
        }
    }
    btstack_core_event_mapper_a2dp_event(&mapped);
}

static void btstack_profile_manager_avrcp_event(
    const btstack_avrcp_event_t *event)
{
    btstack_avrcp_event_t mapped;
    bt_xstack_addr_t address;
    btstack_avrcp_state_t state;
    uint8_t value;

    if (event == NULL) return;
    mapped = *event;
    btstack_profile_manager_resolve_peer(
        BT_XSTACK_PROFILE_AVRCP, mapped.address, &address);
    memcpy(mapped.address, address.bytes, BT_XSTACK_ADDRESS_LEN);

    if (btstack_profile_manager_avrcp_event_state(event->type, &state)) {
        value = btstack_profile_manager_avrcp_state_value(state);
        btstack_platform_enter_critical();
        manager_classic_state.avrcp_state = value;
        btstack_platform_exit_critical();
        if (event->type == BTSTACK_AVRCP_EVENT_DISCONNECTED) {
            (void)btstack_core_state_take_disconnect_request(
                BT_XSTACK_PROFILE_AVRCP);
        }
        btstack_core_event_mapper_post_connection(
            BT_XSTACK_PROFILE_AVRCP,
            (value == 2u) ? BT_XSTACK_CONNECTION_CONNECTING :
                ((value == 3u) ? BT_XSTACK_CONNECTION_CONNECTED :
                    BT_XSTACK_CONNECTION_DISCONNECTED),
            &address, false);
        if (event->type == BTSTACK_AVRCP_EVENT_CONNECTED) {
            (void)btstack_avrcp_request_metadata();
        }
    }

    switch (event->type) {
    case BTSTACK_AVRCP_EVENT_PLAY_STATUS:
        btstack_platform_enter_critical();
        manager_classic_state.play_state = (uint8_t)event->values[0];
        btstack_platform_exit_critical();
        break;
    case BTSTACK_AVRCP_EVENT_PLAY_MODE:
        btstack_platform_enter_critical();
        if (event->value_count > 0u) {
            manager_classic_state.repeat_mode =
                (uint8_t)event->values[0];
        }
        if (event->value_count > 1u) {
            manager_classic_state.shuffle_mode =
                (uint8_t)event->values[1];
        }
        btstack_platform_exit_critical();
        break;
    case BTSTACK_AVRCP_EVENT_TRACK_CHANGED:
        (void)btstack_avrcp_request_metadata();
        break;
    case BTSTACK_AVRCP_EVENT_TRACK_STATUS:
        if (event->value_count > 0u) {
            btstack_platform_enter_critical();
            manager_classic_state.play_state =
                (uint8_t)event->values[0];
            btstack_platform_exit_critical();
        }
        break;
    case BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME:
        if (event->value_count > 0u) {
            btstack_platform_enter_critical();
            manager_classic_state.absolute_volume =
                (uint8_t)event->values[0];
            btstack_platform_exit_critical();
        }
        break;
    default:
        break;
    }
    btstack_core_event_mapper_avrcp_event(&mapped);
}
#endif

static int btstack_profile_manager_text_equal(
    const char *left,
    const char *right)
{
    if (left == right) return 1;
    if ((left == NULL) || (right == NULL)) return 0;
    return strcmp(left, right) == 0;
}

static int btstack_profile_manager_config_equal(
    const btstack_profile_manager_config_t *left,
    const btstack_profile_manager_config_t *right)
{
    return (left->struct_size == right->struct_size) &&
        btstack_profile_manager_text_equal(
            left->device_name, right->device_name) &&
        btstack_profile_manager_text_equal(
            left->manufacturer, right->manufacturer) &&
        (left->classic_profile_mask == right->classic_profile_mask) &&
        (left->le_profile_mask == right->le_profile_mask) &&
        (memcmp(&left->classic, &right->classic,
            sizeof(left->classic)) == 0);
}

static int btstack_profile_manager_validate(
    const btstack_profile_manager_config_t *config)
{
    if ((config == NULL) ||
        (config->struct_size != sizeof(*config)) ||
        (config->device_name == NULL) ||
        (config->manufacturer == NULL) ||
        ((config->classic_profile_mask &
            ~BTSTACK_PROFILE_MANAGER_CLASSIC_ALL) != 0u) ||
        ((config->le_profile_mask & ~BTSTACK_PROFILE_MANAGER_LE_ALL) != 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if ((!BTSTACK_LE && (config->le_profile_mask != 0u)) ||
        (!BTSTACK_GATT_CLIENT && ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT) != 0u)) ||
        (!BTSTACK_GATT_SERVER && ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER) != 0u)) ||
        (!BTSTACK_HOGP_DEVICE && ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u))) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }
    if (((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) &&
        ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER) == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if ((!BTSTACK_CLASSIC && (config->classic_profile_mask != 0u)) ||
        (!BTSTACK_A2DP_SINK && ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_A2DP_SINK) != 0u)) ||
        (!(BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET) &&
            ((config->classic_profile_mask &
                BTSTACK_PROFILE_MANAGER_CLASSIC_AVRCP) != 0u)) ||
        (!BTSTACK_HFP_HF && ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF) != 0u)) ||
        (!BTSTACK_PBAP_CLIENT && ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_PBAP_CLIENT) != 0u))) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }
    if (((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF) != 0u) &&
        (config->classic.hfp_rfcomm_channel == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    return BTSTACK_STATUS_OK;
}

#if BTSTACK_GATT_CLIENT
static void btstack_profile_manager_gatt_client_event(const btstack_gatt_client_event_t *event)
{
    if ((event != NULL) && (event->type == BTSTACK_GATT_CLIENT_EVENT_MTU)) {
        btstack_platform_enter_critical();
        manager_le_state.mtu = event->mtu;
        btstack_platform_exit_critical();
    }
    btstack_core_event_mapper_gatt_client(event);
}
#endif

#if BTSTACK_GATT_SERVER
static void btstack_profile_manager_gatt_server_event(const btstack_gatt_server_event_t *event)
{
#if BTSTACK_HOGP_DEVICE
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) {
        btstack_hogp_device_handle_server_event(event);
    }
#else
    (void)event;
#endif
    btstack_core_event_mapper_gatt_server(event);
}
#endif

static int btstack_profile_manager_setup_le(const btstack_profile_manager_config_t *config)
{
    int status;

#if BTSTACK_GATT_CLIENT
    if ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT) != 0u) {
        status = btstack_gatt_client_init();
        if (status != BTSTACK_STATUS_OK) return status;
        status = btstack_gatt_client_set_event_callback(btstack_profile_manager_gatt_client_event);
        if (status != BTSTACK_STATUS_OK) return status;
    }
#endif
#if BTSTACK_GATT_SERVER
    if ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER) != 0u) {
        status = btstack_gatt_server_init();
        if (status != BTSTACK_STATUS_OK) return status;
        status = btstack_gatt_server_set_event_callback(btstack_profile_manager_gatt_server_event);
        if (status != BTSTACK_STATUS_OK) return status;
    }
#endif
#if BTSTACK_HOGP_DEVICE
    if ((config->le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) {
        status = btstack_hogp_device_init();
        if (status != BTSTACK_STATUS_OK) return status;
    }
#endif
    (void)config;
    (void)status;
    return BTSTACK_STATUS_OK;
}

static int btstack_profile_manager_setup_classic(
    const btstack_profile_manager_config_t *config)
{
#if BTSTACK_CLASSIC
    btstack_a2dp_sink_config_t a2dp_config;
    btstack_avrcp_config_t avrcp_config;
    btstack_hfp_hf_config_t hfp_config;
    int status;

    if (config->classic_profile_mask == 0u) return BTSTACK_STATUS_OK;

    sdp_init();
    rfcomm_init();

    if ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_PBAP_CLIENT) != 0u) {
        status = btstack_pbap_client_setup();
        if (status != BTSTACK_STATUS_OK) return status;
        status = btstack_pbap_client_set_event_callback(
            btstack_profile_manager_pbap_event);
        if (status != BTSTACK_STATUS_OK) return status;
    }

    if ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_A2DP_SINK) != 0u) {
        memset(&a2dp_config, 0, sizeof(a2dp_config));
        a2dp_config.service_name = "A2DP Sink";
        a2dp_config.manufacturer = config->manufacturer;
        a2dp_config.sdp_handle = config->classic.a2dp_sdp_handle;
        a2dp_config.event_callback =
            btstack_profile_manager_a2dp_event;
        a2dp_config.audio_sample_callback =
            btstack_core_event_mapper_media_audio;
        status = btstack_a2dp_sink_setup(&a2dp_config);
        if (status != BTSTACK_STATUS_OK) return status;
    }

    if ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_AVRCP) != 0u) {
        memset(&avrcp_config, 0, sizeof(avrcp_config));
        avrcp_config.controller_service_name = "AVRCP Controller";
        avrcp_config.target_service_name = "AVRCP Target";
        avrcp_config.manufacturer = config->manufacturer;
        avrcp_config.controller_sdp_handle =
            config->classic.avrcp_controller_sdp_handle;
        avrcp_config.target_sdp_handle =
            config->classic.avrcp_target_sdp_handle;
        avrcp_config.event_callback =
            btstack_profile_manager_avrcp_event;
        status = btstack_avrcp_setup(&avrcp_config);
        if (status != BTSTACK_STATUS_OK) return status;
#if BTSTACK_AVRCP_COVER_ART
        status = btstack_avrcp_cover_art_setup(btstack_core_event_mapper_cover_art);
        if (status != BTSTACK_STATUS_OK) return status;
#endif
    }

    if ((config->classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF) != 0u) {
        memset(&hfp_config, 0, sizeof(hfp_config));
        hfp_config.service_name = "Hands-Free";
        hfp_config.manufacturer = config->manufacturer;
        hfp_config.sdp_handle = config->classic.hfp_sdp_handle;
        hfp_config.rfcomm_channel = config->classic.hfp_rfcomm_channel;
        hfp_config.microphone_gain = config->classic.hfp_microphone_gain;
        hfp_config.speaker_gain = config->classic.hfp_speaker_gain;
        hfp_config.event_callback =
            btstack_profile_manager_hfp_event;// 事件上行
        status = btstack_hfp_hf_setup(&hfp_config);
        if (status != BTSTACK_STATUS_OK) return status;
        btstack_call_audio_tx_init();
        status = btstack_hfp_hf_set_direct_audio_callback(
            btstack_core_event_mapper_call_audio);// 下行 PCM
        if (status != BTSTACK_STATUS_OK) return status;
    }

    status = btstack_adapter_set_name(config->device_name);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_adapter_set_class_of_device(
        config->classic.class_of_device);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_adapter_set_ssp_mode(2u);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_adapter_set_connectable(true);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_adapter_set_discoverable(true);
    if (status != BTSTACK_STATUS_OK) return status;
    return BTSTACK_STATUS_OK;
#else
    return (config->classic_profile_mask == 0u) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_profile_manager_init(
    const btstack_profile_manager_config_t *config)
{
    int status = btstack_profile_manager_validate(config);
    if (status != BTSTACK_STATUS_OK) return status;

    btstack_platform_enter_critical();
    if (manager_lifecycle == BTSTACK_PROFILE_MANAGER_READY) {
        status = btstack_profile_manager_config_equal(
            &manager_config, config) ? BTSTACK_STATUS_OK :
            BTSTACK_STATUS_INVALID_ARGUMENT;
        btstack_platform_exit_critical();
        return status;
    }
    if (manager_lifecycle == BTSTACK_PROFILE_MANAGER_INITIALIZING) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_BUSY;
    }
    if (manager_lifecycle == BTSTACK_PROFILE_MANAGER_FAILED) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_NOT_READY;
    }
    manager_config = *config;
    memset(&manager_classic_state, 0, sizeof(manager_classic_state));
    memset(&manager_le_state, 0, sizeof(manager_le_state));
    manager_le_state.connection_handle = 0xffffu;
    memset(manager_profile_peers, 0, sizeof(manager_profile_peers));
    memset(manager_profile_peer_valid, 0,
        sizeof(manager_profile_peer_valid));
    memset(manager_disconnect_requested, 0,
        sizeof(manager_disconnect_requested));
    manager_classic_state.hfp_config = 3u;
    manager_classic_state.hfp_state = BT_XSTACK_HFP_STATE_IDLE;
    manager_classic_state.speaker_volume = 14u;
    manager_classic_state.a2dp_state = BT_XSTACK_A2DP_STATE_IDLE;
    manager_classic_state.avrcp_state = BT_XSTACK_AVRCP_STATE_IDLE;
    manager_classic_state.pbap_state = BT_XSTACK_PHONEBOOK_STATE_IDLE;
    manager_lifecycle = BTSTACK_PROFILE_MANAGER_INITIALIZING;
    btstack_platform_exit_critical();

    status = btstack_profile_manager_setup_classic(config);
    if (status == BTSTACK_STATUS_OK) {
        status = btstack_profile_manager_setup_le(config);
    }

    btstack_platform_enter_critical();
    manager_lifecycle = (status == BTSTACK_STATUS_OK) ?
        BTSTACK_PROFILE_MANAGER_READY : BTSTACK_PROFILE_MANAGER_FAILED;
    btstack_platform_exit_critical();
    return status;
}

int btstack_profile_manager_get_classic_state(
    btstack_profile_manager_classic_state_t *state)
{
    if (state == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    btstack_platform_enter_critical();
    *state = manager_classic_state;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_profile_manager_get_le_state(btstack_profile_manager_le_state_t *state)
{
    if (state == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    btstack_platform_enter_critical();
    *state = manager_le_state;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

void btstack_profile_manager_le_connected(uint16_t connection_handle, uint8_t role,
    uint8_t address_type, const btstack_bd_addr_t address)
{
    if (address == NULL) return;
    btstack_platform_enter_critical();
    manager_le_state.connection_handle = connection_handle;
    manager_le_state.role = role;
    manager_le_state.address_type = address_type;
    memcpy(manager_le_state.peer_address, address, sizeof(manager_le_state.peer_address));
    manager_le_state.connected = true;
    manager_le_state.encrypted = false;
    manager_le_state.bonded = false;
    btstack_platform_exit_critical();
#if BTSTACK_GATT_CLIENT
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT) != 0u) {
        btstack_gatt_client_connection_changed(connection_handle, true);
    }
#endif
#if BTSTACK_GATT_SERVER
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER) != 0u) {
        btstack_gatt_server_connection_changed(connection_handle, true);
    }
#endif
#if BTSTACK_HOGP_DEVICE
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) {
        btstack_hogp_device_connection_changed(connection_handle, true, false);
    }
#endif
}

void btstack_profile_manager_le_security_changed(bool encrypted, bool bonded)
{
    btstack_platform_enter_critical();
    manager_le_state.encrypted = encrypted;
    manager_le_state.bonded = bonded;
    btstack_platform_exit_critical();
#if BTSTACK_HOGP_DEVICE
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) {
        btstack_hogp_device_security_changed(encrypted);
    }
#endif
}

void btstack_profile_manager_le_disconnected(uint16_t connection_handle)
{
    bool matches;

    btstack_platform_enter_critical();
    matches = manager_le_state.connected && (manager_le_state.connection_handle == connection_handle);
    if (matches) {
        memset(&manager_le_state, 0, sizeof(manager_le_state));
        manager_le_state.connection_handle = 0xffffu;
    }
    btstack_platform_exit_critical();
    if (!matches) return;
#if BTSTACK_GATT_CLIENT
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT) != 0u) {
        btstack_gatt_client_connection_changed(connection_handle, false);
    }
#endif
#if BTSTACK_GATT_SERVER
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER) != 0u) {
        btstack_gatt_server_connection_changed(connection_handle, false);
    }
#endif
#if BTSTACK_HOGP_DEVICE
    if ((manager_config.le_profile_mask & BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE) != 0u) {
        btstack_hogp_device_connection_changed(connection_handle, false, false);
    }
#endif
}

int btstack_profile_manager_connect(
    bt_xstack_profile_t profile,
    const bt_xstack_addr_t *address)
{
    bt_xstack_addr_t selected;
    btstack_bd_addr_t native_address;
    uint32_t required_mask;
    int status;

    btstack_platform_enter_critical();
    status = (manager_lifecycle == BTSTACK_PROFILE_MANAGER_READY) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_NOT_READY;
    btstack_platform_exit_critical();
    if (status != BTSTACK_STATUS_OK) return status;

    if (address != NULL) {
        selected = *address;
        btstack_core_state_set_profile_peer(profile, &selected);
    } else if (btstack_core_state_get_profile_peer(
            profile, &selected) != 0) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    memcpy(native_address, selected.bytes, sizeof(native_address));

    switch (profile) {
    case BT_XSTACK_PROFILE_HFP_HF:
        required_mask = BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF;
        break;
    case BT_XSTACK_PROFILE_A2DP_SINK:
        required_mask = BTSTACK_PROFILE_MANAGER_CLASSIC_A2DP_SINK;
        break;
    case BT_XSTACK_PROFILE_AVRCP:
        required_mask = BTSTACK_PROFILE_MANAGER_CLASSIC_AVRCP;
        break;
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if ((manager_config.classic_profile_mask & required_mask) == 0u) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }
    switch (profile) {
    case BT_XSTACK_PROFILE_HFP_HF:
        return btstack_hfp_hf_connect(native_address);
    case BT_XSTACK_PROFILE_A2DP_SINK:
        return btstack_a2dp_sink_connect(native_address);
    case BT_XSTACK_PROFILE_AVRCP:
        return btstack_avrcp_connect(native_address);
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
}

int btstack_profile_manager_disconnect(bt_xstack_profile_t profile)
{
    int status;

    btstack_core_state_mark_disconnect(profile, true);
    switch (profile) {
    case BT_XSTACK_PROFILE_HFP_HF:
        status = btstack_hfp_hf_is_connected() ?
            btstack_hfp_hf_disconnect() : BTSTACK_STATUS_NOT_READY;
        break;
    case BT_XSTACK_PROFILE_A2DP_SINK:
        status = btstack_a2dp_sink_is_connected() ?
            btstack_a2dp_sink_disconnect() : BTSTACK_STATUS_NOT_READY;
        break;
    case BT_XSTACK_PROFILE_AVRCP:
        status = btstack_avrcp_is_connected() ?
            btstack_avrcp_disconnect() : BTSTACK_STATUS_NOT_READY;
        break;
    default:
        btstack_core_state_mark_disconnect(profile, false);
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (status != BTSTACK_STATUS_OK) {
        btstack_core_state_mark_disconnect(profile, false);
    }
    return status;
}

int btstack_profile_manager_disconnect_all(void)
{
    if (btstack_hfp_hf_is_connected()) {
        (void)btstack_profile_manager_disconnect(
            BT_XSTACK_PROFILE_HFP_HF);
    }
    if (btstack_a2dp_sink_is_connected()) {
        (void)btstack_profile_manager_disconnect(
            BT_XSTACK_PROFILE_A2DP_SINK);
    }
    if (btstack_avrcp_is_connected()) {
        (void)btstack_profile_manager_disconnect(
            BT_XSTACK_PROFILE_AVRCP);
    }
    return btstack_adapter_disconnect();
}

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type,
    const uint8_t address[6])
{
    bool linked;
    if (address == NULL) return false;
    btstack_platform_enter_critical();
    linked = (manager_lifecycle == BTSTACK_PROFILE_MANAGER_READY) &&
        (event_type ==
            (uint8_t)BTSTACK_HFP_HF_EVENT_CONNECTION_LOST) &&
        ((manager_config.classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF) != 0u) &&
        ((manager_config.classic_profile_mask &
            BTSTACK_PROFILE_MANAGER_CLASSIC_PBAP_CLIENT) != 0u);
    btstack_platform_exit_critical();
    return linked;
}

void btstack_profile_manager_state_set_value(
    bt_xstack_request_id_t id,
    uint32_t value)
{
    switch (id) {
    case BT_XSTACK_REQUEST_SET_HFP_CONFIG:
    case BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE:
    case BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME:
    case BT_XSTACK_REQUEST_SET_A2DP_MUTE:
    case BT_XSTACK_REQUEST_SET_REPEAT:
    case BT_XSTACK_REQUEST_SET_SHUFFLE:
        break;
    default:
        return;
    }
    btstack_platform_enter_critical();
    switch (id) {
    case BT_XSTACK_REQUEST_SET_HFP_CONFIG:
        manager_classic_state.hfp_config = (uint8_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE:
        manager_classic_state.microphone_muted = (uint8_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME:
        manager_classic_state.speaker_volume = (uint8_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_A2DP_MUTE:
        manager_classic_state.a2dp_muted = (uint8_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_REPEAT:
        manager_classic_state.repeat_mode = (uint8_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_SHUFFLE:
        manager_classic_state.shuffle_mode = (uint8_t)value;
        break;
    default:
        break;
    }
    btstack_platform_exit_critical();
}

void btstack_profile_manager_state_write_pbap(uint8_t state)
{
    btstack_platform_enter_critical();
    manager_classic_state.pbap_state = state;
    btstack_platform_exit_critical();
}

void btstack_core_state_set_profile_peer(
    bt_xstack_profile_t profile,
    const bt_xstack_addr_t *address)
{
    int index = btstack_profile_manager_profile_index(profile);
    if ((index < 0) || (address == NULL)) return;
    btstack_platform_enter_critical();
    manager_profile_peers[index] = *address;
    manager_profile_peer_valid[index] = 1u;
    btstack_platform_exit_critical();
}

int btstack_core_state_get_profile_peer(
    bt_xstack_profile_t profile,
    bt_xstack_addr_t *address)
{
    int index = btstack_profile_manager_profile_index(profile);
    int valid;
    if ((index < 0) || (address == NULL)) return -1;
    btstack_platform_enter_critical();
    valid = manager_profile_peer_valid[index] != 0u;
    if (valid) *address = manager_profile_peers[index];
    btstack_platform_exit_critical();
    return valid ? 0 : btstack_core_state_get_last_peer(address);
}

void btstack_core_state_mark_disconnect(
    bt_xstack_profile_t profile,
    bool requested)
{
    int index = btstack_profile_manager_profile_index(profile);
    if (index < 0) return;
    btstack_platform_enter_critical();
    manager_disconnect_requested[index] = requested ? 1u : 0u;
    btstack_platform_exit_critical();
}

bool btstack_core_state_take_disconnect_request(
    bt_xstack_profile_t profile)
{
    int index = btstack_profile_manager_profile_index(profile);
    bool requested;
    if (index < 0) return false;
    btstack_platform_enter_critical();
    requested = manager_disconnect_requested[index] != 0u;
    manager_disconnect_requested[index] = 0u;
    btstack_platform_exit_critical();
    return requested;
}
