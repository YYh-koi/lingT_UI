#include "btstack_avrcp_internal.h"

#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/avrcp_target.h"
#include "classic/sdp_server.h"

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_TARGET
#define BTSTACK_AVRCP_TARGET_SDP_RECORD_MAX 200u

static uint8_t btstack_avrcp_target_sdp_record[
    BTSTACK_AVRCP_TARGET_SDP_RECORD_MAX];

static void btstack_avrcp_target_packet_handler(
    uint8_t packet_type,
    uint16_t channel,
    uint8_t *packet,
    uint16_t size)
{
    uint8_t subevent;
    uint32_t values[1];
    uint16_t avrcp_cid;

    (void)channel;
    (void)size;
    if ((packet_type != HCI_EVENT_PACKET) ||
        (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META)) {
        return;
    }
    subevent = hci_event_avrcp_meta_get_subevent_code(packet);
    switch (subevent) {
    case AVRCP_SUBEVENT_PLAY_STATUS_QUERY:
        avrcp_cid =
            avrcp_subevent_play_status_query_get_avrcp_cid(packet);
        (void)avrcp_target_play_status(
            avrcp_cid, 0u, 0u, AVRCP_PLAYBACK_STATUS_STOPPED);
        return;

    case AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED:
        values[0] =
            avrcp_subevent_notification_volume_changed_get_absolute_volume(
                packet);
        btstack_avrcp_internal_emit(
            BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME, values, 1u);
        return;

    default:
        return;
    }
}
#endif

int btstack_avrcp_target_setup(
    const btstack_avrcp_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_TARGET
    uint8_t status;
    uint16_t target_features;

    avrcp_target_init();
    avrcp_target_register_packet_handler(
        btstack_avrcp_target_packet_handler);

    target_features =
        (uint16_t)(1u <<
            AVRCP_TARGET_SUPPORTED_FEATURE_CATEGORY_MONITOR_OR_AMPLIFIER);
    avrcp_target_create_sdp_record(
        btstack_avrcp_target_sdp_record,
        config->target_sdp_handle,
        target_features,
        config->target_service_name,
        config->manufacturer);
    status = sdp_register_service(btstack_avrcp_target_sdp_record);
    return (status == ERROR_CODE_SUCCESS) ?
        BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

void btstack_avrcp_target_connected(uint16_t avrcp_cid)
{
#if BTSTACK_CLASSIC && BTSTACK_AVRCP_TARGET
    (void)avrcp_target_support_event(
        avrcp_cid,
        AVRCP_NOTIFICATION_EVENT_VOLUME_CHANGED);
#else
    (void)avrcp_cid;
#endif
}

void btstack_avrcp_target_disconnected(void)
{
}
