#include "btstack_avrcp_internal.h"
#include "profiles/btstack_avrcp_cover_art.h"

#include <string.h>

#include "btstack_defines.h"
#include "btstack_event.h"
#include "btstack_run_loop.h"
#include "classic/avrcp_controller.h"
#include "classic/sdp_server.h"

#define BTSTACK_AVRCP_CONTROLLER_SDP_RECORD_MAX 200u
#define BTSTACK_AVRCP_PENDING_METADATA (1u << 0)
#define BTSTACK_AVRCP_PENDING_PLAY_STATUS (1u << 1)
#define BTSTACK_AVRCP_REQUEST_RETRY_MS 20u
#define BTSTACK_AVRCP_METADATA_RETRY_MS 200u
#define BTSTACK_AVRCP_METADATA_MAX_RETRIES 3u

enum {
    BTSTACK_AVRCP_PROGRESS_INTERVAL_MS = 1000u
};

static uint8_t btstack_avrcp_play_status = AVRCP_PLAYBACK_STATUS_STOPPED;
static char btstack_avrcp_title[BTSTACK_AVRCP_TEXT_MAX];
static char btstack_avrcp_artist[BTSTACK_AVRCP_TEXT_MAX];
static char btstack_avrcp_album[BTSTACK_AVRCP_TEXT_MAX];

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
static uint8_t btstack_avrcp_controller_sdp_record[
    BTSTACK_AVRCP_CONTROLLER_SDP_RECORD_MAX];
static btstack_timer_source_t btstack_avrcp_progress_timer;
static btstack_timer_source_t btstack_avrcp_request_retry_timer;
static uint8_t btstack_avrcp_pending_requests;
static bool btstack_avrcp_metadata_inflight;
static uint8_t btstack_avrcp_metadata_retry_count;

static int btstack_avrcp_controller_status(uint8_t status)
{
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
}

static void btstack_avrcp_copy_bytes(
    char destination[BTSTACK_AVRCP_TEXT_MAX],
    const uint8_t *source,
    uint8_t length)
{
    uint8_t index;

    if (length >= BTSTACK_AVRCP_TEXT_MAX) {
        length = BTSTACK_AVRCP_TEXT_MAX - 1u;
    }
    for (index = 0u; index < length; index++) {
        destination[index] = (char)source[index];
    }
    destination[length] = '\0';
}

static void btstack_avrcp_clear_pending_requests(void)
{
    btstack_avrcp_pending_requests = 0u;
    btstack_avrcp_metadata_inflight = false;
    btstack_avrcp_metadata_retry_count = 0u;
    btstack_run_loop_remove_timer(&btstack_avrcp_request_retry_timer);
}

static void btstack_avrcp_try_pending_request(void);

static void btstack_avrcp_request_retry_timeout(
    btstack_timer_source_t *timer)
{
    (void)timer;
    btstack_avrcp_try_pending_request();
}

static void btstack_avrcp_arm_request_retry(uint32_t delay_ms)
{
    btstack_run_loop_remove_timer(&btstack_avrcp_request_retry_timer);
    btstack_run_loop_set_timer_handler(
        &btstack_avrcp_request_retry_timer,
        btstack_avrcp_request_retry_timeout);
    btstack_run_loop_set_timer(
        &btstack_avrcp_request_retry_timer,
        delay_ms);
    btstack_run_loop_add_timer(&btstack_avrcp_request_retry_timer);
}

static void btstack_avrcp_schedule_request(uint8_t request_mask)
{
    btstack_avrcp_pending_requests |= request_mask;
    btstack_avrcp_arm_request_retry(BTSTACK_AVRCP_REQUEST_RETRY_MS);
}

static void btstack_avrcp_metadata_complete(uint8_t status)
{
    uint16_t avrcp_cid = btstack_avrcp_internal_get_cid();

    btstack_avrcp_metadata_inflight = false;

    if (status == ERROR_CODE_SUCCESS) {
        btstack_avrcp_metadata_retry_count = 0u;
        if (btstack_avrcp_pending_requests != 0u) {
            btstack_avrcp_arm_request_retry(
                BTSTACK_AVRCP_REQUEST_RETRY_MS);
        }
        return;
    }

    if ((avrcp_cid != 0u) &&
        (btstack_avrcp_metadata_retry_count <
            BTSTACK_AVRCP_METADATA_MAX_RETRIES)) {
        btstack_avrcp_metadata_retry_count++;
        btstack_avrcp_pending_requests |=
            BTSTACK_AVRCP_PENDING_METADATA;
        btstack_avrcp_arm_request_retry(
            BTSTACK_AVRCP_METADATA_RETRY_MS);
    } else {
        btstack_avrcp_metadata_retry_count = 0u;
    }
}

static void btstack_avrcp_try_pending_request(void)
{
    uint8_t selected_request;
    uint8_t status;
    uint16_t avrcp_cid = btstack_avrcp_internal_get_cid();

    if (avrcp_cid == 0u) {
        btstack_avrcp_clear_pending_requests();
        return;
    }

    if ((btstack_avrcp_pending_requests &
            BTSTACK_AVRCP_PENDING_METADATA) != 0u) {
        if (btstack_avrcp_metadata_inflight) {
            btstack_run_loop_remove_timer(
                &btstack_avrcp_request_retry_timer);
            return;
        }
        selected_request = BTSTACK_AVRCP_PENDING_METADATA;
        status = avrcp_controller_get_now_playing_info(avrcp_cid);
    } else if ((btstack_avrcp_pending_requests &
                   BTSTACK_AVRCP_PENDING_PLAY_STATUS) != 0u) {
        selected_request = BTSTACK_AVRCP_PENDING_PLAY_STATUS;
        status = avrcp_controller_get_play_status(avrcp_cid);
    } else {
        btstack_run_loop_remove_timer(
            &btstack_avrcp_request_retry_timer);
        return;
    }

    if (status == ERROR_CODE_SUCCESS) {
        if (selected_request == BTSTACK_AVRCP_PENDING_METADATA) {
            btstack_avrcp_metadata_inflight = true;
        }
        btstack_avrcp_pending_requests &=
            (uint8_t)~selected_request;
    } else if (status != ERROR_CODE_COMMAND_DISALLOWED) {
        btstack_avrcp_pending_requests &=
            (uint8_t)~selected_request;
        if (selected_request == BTSTACK_AVRCP_PENDING_METADATA) {
            btstack_avrcp_metadata_complete(status);
            return;
        }
    }

    if (btstack_avrcp_pending_requests != 0u) {
        btstack_avrcp_arm_request_retry(BTSTACK_AVRCP_REQUEST_RETRY_MS);
    }
}

static void btstack_avrcp_progress_timeout(
    btstack_timer_source_t *timer)
{
    (void)timer;
    if ((btstack_avrcp_internal_get_cid() == 0u) ||
        (btstack_avrcp_play_status !=
            AVRCP_PLAYBACK_STATUS_PLAYING)) {
        return;
    }
    btstack_avrcp_schedule_request(
        BTSTACK_AVRCP_PENDING_PLAY_STATUS);
    btstack_run_loop_set_timer(
        &btstack_avrcp_progress_timer,
        BTSTACK_AVRCP_PROGRESS_INTERVAL_MS);
    btstack_run_loop_add_timer(&btstack_avrcp_progress_timer);
}

static void btstack_avrcp_update_progress_timer(void)
{
    btstack_run_loop_remove_timer(&btstack_avrcp_progress_timer);
    if ((btstack_avrcp_internal_get_cid() == 0u) ||
        (btstack_avrcp_play_status !=
            AVRCP_PLAYBACK_STATUS_PLAYING)) {
        return;
    }
    btstack_run_loop_set_timer_handler(
        &btstack_avrcp_progress_timer,
        btstack_avrcp_progress_timeout);
    btstack_run_loop_set_timer(
        &btstack_avrcp_progress_timer,
        BTSTACK_AVRCP_PROGRESS_INTERVAL_MS);
    btstack_run_loop_add_timer(&btstack_avrcp_progress_timer);
}

static void btstack_avrcp_controller_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    uint8_t subevent;
    uint8_t status;
    const uint8_t *value;
    uint8_t length;
    uint32_t values[3];

    (void)channel;
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)) {
        return;
    }
    subevent = hci_event_avrcp_meta_get_subevent_code(packet);
    switch (subevent) {
    case AVRCP_SUBEVENT_NOTIFICATION_PLAYBACK_STATUS_CHANGED:
        btstack_avrcp_play_status =
            avrcp_subevent_notification_playback_status_changed_get_play_status(
                packet);
        btstack_avrcp_update_progress_timer();
        values[0] = btstack_avrcp_play_status;
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_PLAY_STATUS, values, 1u);
        return;

    case AVRCP_SUBEVENT_PLAY_STATUS:
        btstack_avrcp_play_status =
            avrcp_subevent_play_status_get_play_status(packet);
        btstack_avrcp_update_progress_timer();
        values[0] = btstack_avrcp_play_status;
        values[1] =
            avrcp_subevent_play_status_get_song_position(packet) / 1000u;
        values[2] =
            avrcp_subevent_play_status_get_song_length(packet) / 1000u;
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_TRACK_STATUS, values, 3u);
        return;

    case AVRCP_SUBEVENT_NOTIFICATION_TRACK_CHANGED:
#if BTSTACK_AVRCP_COVER_ART
        btstack_avrcp_cover_art_track_changed();
#endif
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_TRACK_CHANGED, NULL, 0u);
        return;

    case AVRCP_SUBEVENT_SHUFFLE_AND_REPEAT_MODE:
        values[0] =
            (avrcp_subevent_shuffle_and_repeat_mode_get_repeat_mode(
                packet) == AVRCP_REPEAT_MODE_OFF) ? 0u : 1u;
        values[1] =
            (avrcp_subevent_shuffle_and_repeat_mode_get_shuffle_mode(
                packet) == AVRCP_SHUFFLE_MODE_OFF) ? 0u : 1u;
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_PLAY_MODE, values, 2u);
        return;

    case AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED:
        values[0] =
            avrcp_subevent_notification_volume_changed_get_absolute_volume(
                packet);
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME, values, 1u);
        return;

    case AVRCP_SUBEVENT_SET_ABSOLUTE_VOLUME_RESPONSE:
        values[0] =
            avrcp_subevent_set_absolute_volume_response_get_absolute_volume(
                packet);
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME, values, 1u);
        return;

    case AVRCP_SUBEVENT_NOW_PLAYING_TITLE_INFO:
        length =
            avrcp_subevent_now_playing_title_info_get_value_len(packet);
        value = avrcp_subevent_now_playing_title_info_get_value(packet);
        btstack_avrcp_copy_bytes(btstack_avrcp_title, value, length);
        return;

    case AVRCP_SUBEVENT_NOW_PLAYING_ARTIST_INFO:
        length =
            avrcp_subevent_now_playing_artist_info_get_value_len(packet);
        value = avrcp_subevent_now_playing_artist_info_get_value(packet);
        btstack_avrcp_copy_bytes(btstack_avrcp_artist, value, length);
        return;

    case AVRCP_SUBEVENT_NOW_PLAYING_ALBUM_INFO:
        length =
            avrcp_subevent_now_playing_album_info_get_value_len(packet);
        value = avrcp_subevent_now_playing_album_info_get_value(packet);
        btstack_avrcp_copy_bytes(btstack_avrcp_album, value, length);
        return;

#if BTSTACK_AVRCP_COVER_ART
    case AVRCP_SUBEVENT_NOW_PLAYING_COVER_ART_INFO:
        length = avrcp_subevent_now_playing_cover_art_info_get_value_len(packet);
        value = avrcp_subevent_now_playing_cover_art_info_get_value(packet);
        btstack_avrcp_cover_art_handle_received(value, length);
        return;
#endif

    case AVRCP_SUBEVENT_NOW_PLAYING_INFO_DONE:
        status = avrcp_subevent_now_playing_info_done_get_status(packet);
        btstack_avrcp_metadata_complete(status);
        if (status == ERROR_CODE_SUCCESS) {
            btstack_avrcp_internal_emit(
                BTSTACK_AVRCP_EVENT_TRACK_INFO, NULL, 0u);
        }
        return;

    case AVRCP_SUBEVENT_OPERATION_COMPLETE:
        if ((size >= 10u) &&
            (packet[7] == AVRCP_PDU_ID_GET_ELEMENT_ATTRIBUTES) &&
            (packet[9] == ERROR_CODE_PAGE_TIMEOUT) &&
            btstack_avrcp_metadata_inflight) {
            btstack_avrcp_metadata_complete(ERROR_CODE_PAGE_TIMEOUT);
        }
        return;

    default:
        return;
    }
}
#endif

int btstack_avrcp_controller_setup(
    const btstack_avrcp_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    uint8_t status;
    uint16_t controller_features;

    btstack_avrcp_play_status = AVRCP_PLAYBACK_STATUS_STOPPED;
    btstack_avrcp_pending_requests = 0u;
    btstack_avrcp_metadata_inflight = false;
    btstack_avrcp_metadata_retry_count = 0u;
    memset(btstack_avrcp_title, 0, sizeof(btstack_avrcp_title));
    memset(btstack_avrcp_artist, 0, sizeof(btstack_avrcp_artist));
    memset(btstack_avrcp_album, 0, sizeof(btstack_avrcp_album));

    avrcp_controller_init();
    avrcp_controller_register_packet_handler(
        btstack_avrcp_controller_packet_handler);

    controller_features =
        (uint16_t)(1u <<
            AVRCP_CONTROLLER_SUPPORTED_FEATURE_CATEGORY_PLAYER_OR_RECORDER);
#if BTSTACK_AVRCP_COVER_ART
    controller_features |= (uint16_t)(1u << AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_IMAGE_PROPERTIES);
    controller_features |= (uint16_t)(1u << AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_IMAGE);
    controller_features |= (uint16_t)(1u << AVRCP_CONTROLLER_SUPPORTED_FEATURE_COVER_ART_GET_LINKED_THUMBNAIL);
#endif
    avrcp_controller_create_sdp_record(
        btstack_avrcp_controller_sdp_record,
        config->controller_sdp_handle,
        controller_features,
        config->controller_service_name,
        config->manufacturer);
    status = sdp_register_service(
        btstack_avrcp_controller_sdp_record);
    return btstack_avrcp_controller_status(status);
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

void btstack_avrcp_controller_connected(uint16_t avrcp_cid)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    (void)avrcp_controller_get_supported_events(avrcp_cid);
    (void)avrcp_controller_enable_notification(
        avrcp_cid,
        AVRCP_NOTIFICATION_EVENT_PLAYBACK_STATUS_CHANGED);
    (void)avrcp_controller_enable_notification(
        avrcp_cid,
        AVRCP_NOTIFICATION_EVENT_TRACK_CHANGED);
    (void)avrcp_controller_get_play_status(avrcp_cid);
    btstack_avrcp_update_progress_timer();
#else
    (void)avrcp_cid;
#endif
}

void btstack_avrcp_controller_disconnected(void)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    btstack_avrcp_clear_pending_requests();
    btstack_avrcp_update_progress_timer();
#endif
}

void btstack_avrcp_controller_copy_metadata(
    btstack_avrcp_event_t *event)
{
    memcpy(event->title, btstack_avrcp_title, sizeof(event->title));
    memcpy(event->artist, btstack_avrcp_artist, sizeof(event->artist));
    memcpy(event->album, btstack_avrcp_album, sizeof(event->album));
}

uint8_t btstack_avrcp_controller_get_play_status(void)
{
    return btstack_avrcp_play_status;
}

int btstack_avrcp_controller_control(
    btstack_avrcp_control_t control)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    uint8_t status;
    uint16_t avrcp_cid = btstack_avrcp_internal_get_cid();

    if (avrcp_cid == 0u) {
        return BTSTACK_STATUS_NOT_READY;
    }
    switch (control) {
    case BTSTACK_AVRCP_CONTROL_PLAY:
        status = avrcp_controller_play(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_PAUSE:
        status = avrcp_controller_pause(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_STOP:
        status = avrcp_controller_stop(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_NEXT:
        status = avrcp_controller_forward(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_PREVIOUS:
        status = avrcp_controller_backward(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_FAST_FORWARD:
        status = avrcp_controller_fast_forward(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_REWIND:
        status = avrcp_controller_rewind(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_VOLUME_UP:
        status = avrcp_controller_volume_up(avrcp_cid);
        break;
    case BTSTACK_AVRCP_CONTROL_VOLUME_DOWN:
        status = avrcp_controller_volume_down(avrcp_cid);
        break;
    default:
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    return btstack_avrcp_controller_status(status);
#else
    (void)control;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_avrcp_controller_set_repeat(bool enabled)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    uint16_t avrcp_cid = btstack_avrcp_internal_get_cid();

    if (avrcp_cid == 0u) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_avrcp_controller_status(
        avrcp_controller_set_repeat_mode(
            avrcp_cid,
            enabled ? AVRCP_REPEAT_MODE_ALL_TRACKS :
                      AVRCP_REPEAT_MODE_OFF));
#else
    (void)enabled;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_avrcp_controller_set_shuffle(bool enabled)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    uint16_t avrcp_cid = btstack_avrcp_internal_get_cid();

    if (avrcp_cid == 0u) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_avrcp_controller_status(
        avrcp_controller_set_shuffle_mode(
            avrcp_cid,
            enabled ? AVRCP_SHUFFLE_MODE_ALL_TRACKS :
                      AVRCP_SHUFFLE_MODE_OFF));
#else
    (void)enabled;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_avrcp_controller_request_metadata(void)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER
    if (btstack_avrcp_internal_get_cid() == 0u) {
        return BTSTACK_STATUS_NOT_READY;
    }
    memset(btstack_avrcp_title, 0, sizeof(btstack_avrcp_title));
    memset(btstack_avrcp_artist, 0, sizeof(btstack_avrcp_artist));
    memset(btstack_avrcp_album, 0, sizeof(btstack_avrcp_album));
    btstack_avrcp_metadata_retry_count = 0u;
    btstack_avrcp_schedule_request(
        BTSTACK_AVRCP_PENDING_METADATA);
    return BTSTACK_STATUS_OK;
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}
