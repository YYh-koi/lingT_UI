#include "profiles/btstack_avrcp.h"

#include <string.h>

#include "btstack_avrcp_internal.h"
#include "profiles/btstack_avrcp_cover_art.h"
#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/avrcp.h"

static btstack_avrcp_config_t btstack_avrcp_config;
static btstack_avrcp_state_t btstack_avrcp_state =
    BTSTACK_AVRCP_STATE_IDLE;
static btstack_bd_addr_t btstack_avrcp_peer_address;
static uint16_t btstack_avrcp_cid;

#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
static bool btstack_avrcp_setup_done;

static int btstack_avrcp_status(uint8_t status)
{
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
}

static void btstack_avrcp_set_state(btstack_avrcp_state_t state)
{
    if (btstack_avrcp_state == state) {
        return;
    }
    btstack_avrcp_state = state;
    btstack_avrcp_internal_emit(
        (btstack_avrcp_event_type_t)state, NULL, 0u);
}

static void btstack_avrcp_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    uint8_t status;

    (void)channel;
    (void)size;
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)) {
        return;
    }
    switch (hci_event_avrcp_meta_get_subevent_code(packet)) {
    case AVRCP_SUBEVENT_CONNECTION_ESTABLISHED:
        status = avrcp_subevent_connection_established_get_status(packet);
        if (status != ERROR_CODE_SUCCESS) {
            btstack_avrcp_cid = 0u;
            btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_IDLE);
            return;
        }
        btstack_avrcp_cid =
            avrcp_subevent_connection_established_get_avrcp_cid(packet);
        avrcp_subevent_connection_established_get_bd_addr(
            packet,
            btstack_avrcp_peer_address);
        btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_CONNECTED);
#if BTSTACK_AVRCP_COVER_ART
        btstack_avrcp_cover_art_connected(btstack_avrcp_peer_address);
#endif
#if BTSTACK_AVRCP_CONTROLLER
        btstack_avrcp_controller_connected(btstack_avrcp_cid);
#endif
#if BTSTACK_AVRCP_TARGET
        btstack_avrcp_target_connected(btstack_avrcp_cid);
#endif
        return;

    case AVRCP_SUBEVENT_CONNECTION_RELEASED:
#if BTSTACK_AVRCP_COVER_ART
        btstack_avrcp_cover_art_disconnected(btstack_avrcp_peer_address);
#endif
        btstack_avrcp_cid = 0u;
#if BTSTACK_AVRCP_CONTROLLER
        btstack_avrcp_controller_disconnected();
#endif
#if BTSTACK_AVRCP_TARGET
        btstack_avrcp_target_disconnected();
#endif
        btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_IDLE);
        return;

    default:
        return;
    }
}
#endif

uint16_t btstack_avrcp_internal_get_cid(void)
{
    return btstack_avrcp_cid;
}

void btstack_avrcp_internal_emit(
    btstack_avrcp_event_type_t type,
    const uint32_t *values,
    uint8_t value_count)
{
    btstack_avrcp_event_t event;
    uint8_t index;

    memset(&event, 0, sizeof(event));
    event.type = type;
    memcpy(event.address, btstack_avrcp_peer_address,
           sizeof(event.address));
    if (value_count > 3u) {
        value_count = 3u;
    }
    for (index = 0u; index < value_count; index++) {
        event.values[index] = values[index];
    }
    event.value_count = value_count;
    btstack_avrcp_controller_copy_metadata(&event);
    if (btstack_avrcp_config.event_callback != NULL) {
        btstack_avrcp_config.event_callback(&event);
    }
}

int btstack_avrcp_setup(const btstack_avrcp_config_t *config)
{
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
    int status;

    if (config == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
#if BTSTACK_AVRCP_CONTROLLER
    if (config->controller_service_name == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
#endif
#if BTSTACK_AVRCP_TARGET
    if (config->target_service_name == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
#endif
    btstack_avrcp_config = *config;
    btstack_avrcp_cid = 0u;
    memset(btstack_avrcp_peer_address, 0,
           sizeof(btstack_avrcp_peer_address));

    avrcp_init();
    avrcp_register_packet_handler(btstack_avrcp_packet_handler);   

#if BTSTACK_AVRCP_CONTROLLER
    status = btstack_avrcp_controller_setup(config);   // 控制器  
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
#endif
#if BTSTACK_AVRCP_TARGET
    status = btstack_avrcp_target_setup(config);  //目标
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
#endif

    btstack_avrcp_setup_done = true;
    btstack_avrcp_state = BTSTACK_AVRCP_STATE_IDLE;
    btstack_avrcp_internal_emit(
        BTSTACK_AVRCP_EVENT_DISCONNECTED, NULL, 0u);
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_avrcp_connect(const btstack_bd_addr_t address)
{
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
    uint8_t status;

    if (!btstack_avrcp_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (address == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_avrcp_state != BTSTACK_AVRCP_STATE_IDLE) {
        return BTSTACK_STATUS_BUSY;
    }
    memcpy(btstack_avrcp_peer_address, address,
           sizeof(btstack_avrcp_peer_address));
    btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_CONNECTING);
    status = avrcp_connect(
        btstack_avrcp_peer_address,
        &btstack_avrcp_cid);
    if (status != ERROR_CODE_SUCCESS) {
        btstack_avrcp_cid = 0u;
        btstack_avrcp_set_state(BTSTACK_AVRCP_STATE_IDLE);
    }
    return btstack_avrcp_status(status);
#else
    (void)address;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_avrcp_disconnect(void)
{
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
    if (btstack_avrcp_cid == 0u) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_avrcp_status(avrcp_disconnect(btstack_avrcp_cid));
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

bool btstack_avrcp_is_connected(void)
{
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
    return btstack_avrcp_state == BTSTACK_AVRCP_STATE_CONNECTED;
#else
    return false;
#endif
}

btstack_avrcp_state_t btstack_avrcp_get_state(void)
{
    return btstack_avrcp_state;
}

uint8_t btstack_avrcp_get_play_status(void)
{
    return btstack_avrcp_controller_get_play_status();
}

int btstack_avrcp_control(btstack_avrcp_control_t control)
{
    return btstack_avrcp_controller_control(control);
}

int btstack_avrcp_execute_control(uint32_t control)
{
    enum {
        AVRCP_REQUEST_PLAY_PAUSE = 0,
        AVRCP_REQUEST_PLAY,
        AVRCP_REQUEST_PAUSE,
        AVRCP_REQUEST_STOP,
        AVRCP_REQUEST_NEXT,
        AVRCP_REQUEST_PREVIOUS,
        AVRCP_REQUEST_FAST_FORWARD,
        AVRCP_REQUEST_REWIND,
        AVRCP_REQUEST_VOLUME_UP,
        AVRCP_REQUEST_VOLUME_DOWN
    };
    uint8_t play_status;

    if (!btstack_avrcp_is_connected()) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (control == AVRCP_REQUEST_PLAY_PAUSE) {
        play_status = btstack_avrcp_get_play_status();
        if ((play_status == AVRCP_PLAYBACK_STATUS_STOPPED) ||
            (play_status == AVRCP_PLAYBACK_STATUS_PAUSED)) {
            return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PLAY);
        }
        if ((play_status == AVRCP_PLAYBACK_STATUS_PLAYING) ||
            (play_status == AVRCP_PLAYBACK_STATUS_FWD_SEEK) ||
            (play_status == AVRCP_PLAYBACK_STATUS_REV_SEEK)) {
            return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PAUSE);
        }
        return BTSTACK_STATUS_NOT_READY;
    }
    switch (control) {
    case AVRCP_REQUEST_PLAY:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PLAY);
    case AVRCP_REQUEST_PAUSE:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PAUSE);
    case AVRCP_REQUEST_STOP:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_STOP);
    case AVRCP_REQUEST_NEXT:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_NEXT);
    case AVRCP_REQUEST_PREVIOUS:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PREVIOUS);
    case AVRCP_REQUEST_FAST_FORWARD:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_FAST_FORWARD);
    case AVRCP_REQUEST_REWIND:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_REWIND);
    case AVRCP_REQUEST_VOLUME_UP:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_VOLUME_UP);
    case AVRCP_REQUEST_VOLUME_DOWN:
        return btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_VOLUME_DOWN);
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
}

int btstack_avrcp_set_repeat(bool enabled)
{
    return btstack_avrcp_controller_set_repeat(enabled);
}

int btstack_avrcp_set_shuffle(bool enabled)
{
    return btstack_avrcp_controller_set_shuffle(enabled);
}

int btstack_avrcp_request_metadata(void)
{
    return btstack_avrcp_controller_request_metadata();
}

int btstack_avrcp_set_event_callback(
    btstack_avrcp_event_callback_t callback)
{
#if BTSTACK_CLASSIC && \
    (BTSTACK_AVRCP_CONTROLLER || BTSTACK_AVRCP_TARGET)
    btstack_avrcp_config.event_callback = callback;
    return BTSTACK_STATUS_OK;
#else
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}
