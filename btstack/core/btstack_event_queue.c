#include "events/btstack_event.h"

#include <string.h>

#include "adapter/btstack_port_ops.h"
#include "port/btstack_platform.h"

enum {
    BTSTACK_HID_REPORT_TYPE_OUTPUT = 2u,
    BTSTACK_HID_REPORT_TYPE_FEATURE = 3u
};

typedef enum {
    BTSTACK_QUEUED_APP_EVENT = 0,
    BTSTACK_QUEUED_HID_REPORT,
    BTSTACK_QUEUED_BLE_GATT_WRITE,
    BTSTACK_QUEUED_A2DP_AUDIO_SAMPLE,
    BTSTACK_QUEUED_A2DP_MEDIA_DATA,
    BTSTACK_QUEUED_HFP_EVENT,
    BTSTACK_QUEUED_HFP_AUDIO_SAMPLE
} btstack_queued_event_kind_t;

typedef struct {
    btstack_queued_event_kind_t kind;
    btstack_transport_t transport;
    btstack_event_t event;
    uint16_t data_len;
    uint8_t data[BTSTACK_EVENT_DATA_MAX];
    union {
        struct {
            uint8_t report_type;
            uint8_t report_id;
        } hid_report;
        struct {
            uint16_t value_handle;
        } gatt_write;
        btstack_audio_sample_t audio_sample;
        btstack_media_packet_t media_packet;
        btstack_hfp_event_t hfp_event;
    } meta;
} btstack_queued_event_t;

static btstack_queued_event_t btstack_event_queue[BTSTACK_EVENT_QUEUE_LEN];
static uint16_t btstack_event_read_index;
static uint16_t btstack_event_write_index;
static uint16_t btstack_event_count;
static bool btstack_event_is_processing;

static btstack_app_event_callback_t btstack_app_callback;
static btstack_hid_get_report_callback_t btstack_hid_get_report_callback;
static btstack_ble_gatt_write_callback_t btstack_ble_gatt_write_callback;
static btstack_audio_sample_callback_t btstack_audio_sample_callback;
static btstack_media_data_callback_t btstack_media_data_callback;
static btstack_audio_sample_callback_t btstack_hfp_audio_sample_callback;
static btstack_hfp_event_callback_t btstack_hfp_callback;

static void btstack_event_signal_worker(void);

static btstack_event_t btstack_event_from_hid_report_type(
    uint8_t report_type)
{
    if (report_type == BTSTACK_HID_REPORT_TYPE_OUTPUT) {
        return BTSTACK_EVENT_HID_OUTPUT_REPORT;
    }

    if (report_type == BTSTACK_HID_REPORT_TYPE_FEATURE) {
        return BTSTACK_EVENT_HID_FEATURE_REPORT;
    }

    return BTSTACK_EVENT_UNKNOWN;
}

static void btstack_event_dispatch(
    const btstack_queued_event_t *item)
{
    btstack_app_event_callback_t callback;
    btstack_ble_gatt_write_callback_t gatt_write_callback;
    btstack_audio_sample_callback_t audio_callback;
    btstack_media_data_callback_t media_callback;
    btstack_hfp_event_callback_t hfp_callback;
    btstack_hid_report_payload_t hid_payload;
    btstack_audio_sample_t audio_sample;
    btstack_media_packet_t media_packet;
    btstack_hfp_event_t hfp_event;

    switch (item->kind) {
    case BTSTACK_QUEUED_APP_EVENT:
        btstack_platform_enter_critical();
        callback = btstack_app_callback;
        btstack_platform_exit_critical();

        if (callback != NULL) {
            callback(
                item->transport,
                item->event,
                (item->data_len > 0u) ? item->data : NULL,
                item->data_len);
        }
        return;

    case BTSTACK_QUEUED_HID_REPORT:
        btstack_platform_enter_critical();
        callback = btstack_app_callback;
        btstack_platform_exit_critical();

        if (callback == NULL) {
            return;
        }

        hid_payload.report_type = item->meta.hid_report.report_type;
        hid_payload.report_id = item->meta.hid_report.report_id;
        hid_payload.report_len = item->data_len;
        hid_payload.report = (item->data_len > 0u) ? item->data : NULL;

        callback(
            item->transport,
            item->event,
            (const uint8_t *)&hid_payload,
            (uint16_t)sizeof(hid_payload));
        return;

    case BTSTACK_QUEUED_BLE_GATT_WRITE:
        btstack_platform_enter_critical();
        gatt_write_callback = btstack_ble_gatt_write_callback;
        btstack_platform_exit_critical();

        if (gatt_write_callback != NULL) {
            (void)gatt_write_callback(
                item->meta.gatt_write.value_handle,
                (item->data_len > 0u) ? item->data : NULL,
                item->data_len);
        }
        return;

    case BTSTACK_QUEUED_A2DP_AUDIO_SAMPLE:
    case BTSTACK_QUEUED_HFP_AUDIO_SAMPLE:
        btstack_platform_enter_critical();
        audio_callback =
            (item->kind == BTSTACK_QUEUED_HFP_AUDIO_SAMPLE) ?
            btstack_hfp_audio_sample_callback :
            btstack_audio_sample_callback;
        btstack_platform_exit_critical();

        if (audio_callback != NULL) {
            audio_sample = item->meta.audio_sample;
            audio_sample.data =
                (item->data_len > 0u) ? item->data : NULL;
            audio_sample.data_len = item->data_len;
            audio_callback(&audio_sample);
        }
        return;

    case BTSTACK_QUEUED_A2DP_MEDIA_DATA:
        btstack_platform_enter_critical();
        media_callback = btstack_media_data_callback;
        btstack_platform_exit_critical();

        if (media_callback != NULL) {
            media_packet = item->meta.media_packet;
            media_packet.data =
                (item->data_len > 0u) ? item->data : NULL;
            media_packet.data_len = item->data_len;
            media_callback(&media_packet);
        }
        return;

    case BTSTACK_QUEUED_HFP_EVENT:
        btstack_platform_enter_critical();
        hfp_callback = btstack_hfp_callback;
        btstack_platform_exit_critical();

        if (hfp_callback != NULL) {
            hfp_event = item->meta.hfp_event;
            hfp_event.text =
                (item->data_len > 0u) ? (const char *)item->data : NULL;
            hfp_callback(&hfp_event);
        }
        return;

    default:
        return;
    }
}

static int btstack_event_enqueue(const btstack_queued_event_t *source)
{
    btstack_queued_event_t *item;

    btstack_platform_enter_critical();
    if (btstack_event_count >= (uint16_t)BTSTACK_EVENT_QUEUE_LEN) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_NO_MEMORY;
    }

    item = &btstack_event_queue[btstack_event_write_index];
    *item = *source;

    btstack_event_write_index++;
    if (btstack_event_write_index >= (uint16_t)BTSTACK_EVENT_QUEUE_LEN) {
        btstack_event_write_index = 0u;
    }
    btstack_event_count++;
    btstack_platform_exit_critical();

    btstack_event_signal_worker();
    return BTSTACK_STATUS_OK;
}

static uint16_t btstack_event_copy_text(
    uint8_t *dst,
    const char *text)
{
    uint16_t i = 0u;

    if (text == NULL) {
        return 0u;
    }

    while ((i < (uint16_t)(BTSTACK_EVENT_DATA_MAX - 1u)) &&
           (text[i] != '\0')) {
        dst[i] = (uint8_t)text[i];
        i++;
    }
    dst[i] = '\0';
    return (uint16_t)(i + 1u);
}

static void btstack_event_signal_worker(void)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();
    bool scheduled = false;

    if (ops == NULL) {
        btstack_event_process();
        return;
    }

    if (ops->system.signal_event != NULL) {
        ops->system.signal_event(ops->system.context);
        scheduled = true;
    }

    if (ops->system.wake_event_worker != NULL) {
        ops->system.wake_event_worker(ops->system.context);
        scheduled = true;
    }

    if (ops->system.schedule_event_worker != NULL) {
        scheduled =
            (ops->system.schedule_event_worker(ops->system.context) ==
             BTSTACK_STATUS_OK);
    }

    if (!scheduled) {
        btstack_event_process();
    }
}

void btstack_event_register_app_callback(
    btstack_app_event_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_app_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_hid_get_report_callback(
    btstack_hid_get_report_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_hid_get_report_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_ble_gatt_write_callback(
    btstack_ble_gatt_write_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_ble_gatt_write_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_a2dp_audio_sample_callback(
    btstack_audio_sample_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_audio_sample_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_a2dp_media_data_callback(
    btstack_media_data_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_media_data_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_hfp_callback(
    btstack_hfp_event_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_hfp_callback = callback;
    btstack_platform_exit_critical();
}

void btstack_event_register_hfp_audio_sample_callback(
    btstack_audio_sample_callback_t callback)
{
    btstack_platform_enter_critical();
    btstack_hfp_audio_sample_callback = callback;
    btstack_platform_exit_critical();
}

int btstack_event_post(
    btstack_transport_t transport,
    btstack_event_t event,
    const uint8_t *data,
    uint16_t data_len)
{
    btstack_queued_event_t item;

    if ((data == NULL) && (data_len > 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    if (data_len > (uint16_t)BTSTACK_EVENT_DATA_MAX) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_APP_EVENT;
    item.transport = transport;
    item.event = event;
    item.data_len = data_len;
    if (data_len > 0u) {
        memcpy(item.data, data, data_len);
    }

    return btstack_event_enqueue(&item);
}

int btstack_event_post_hid_report(
    btstack_transport_t transport,
    uint8_t report_type,
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len)
{
    btstack_queued_event_t item;

    if ((report == NULL) && (report_len > 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    if (report_len > (uint16_t)BTSTACK_EVENT_DATA_MAX) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    if (btstack_event_from_hid_report_type(report_type) ==
        BTSTACK_EVENT_UNKNOWN) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_HID_REPORT;
    item.transport = transport;
    item.event = btstack_event_from_hid_report_type(report_type);
    item.data_len = report_len;
    item.meta.hid_report.report_type = report_type;
    item.meta.hid_report.report_id = report_id;
    if (report_len > 0u) {
        memcpy(item.data, report, report_len);
    }

    return btstack_event_enqueue(&item);
}

void btstack_event_process(void)
{
    btstack_queued_event_t item;

    btstack_platform_enter_critical();
    if (btstack_event_is_processing) {
        btstack_platform_exit_critical();
        return;
    }
    btstack_event_is_processing = true;
    btstack_platform_exit_critical();

    for (;;) {
        btstack_platform_enter_critical();
        if (btstack_event_count == 0u) {
            btstack_event_is_processing = false;
            btstack_platform_exit_critical();
            return;
        }

        item = btstack_event_queue[btstack_event_read_index];
        memset(
            &btstack_event_queue[btstack_event_read_index],
            0,
            sizeof(btstack_event_queue[btstack_event_read_index]));

        btstack_event_read_index++;
        if (btstack_event_read_index >= (uint16_t)BTSTACK_EVENT_QUEUE_LEN) {
            btstack_event_read_index = 0u;
        }
        btstack_event_count--;
        btstack_platform_exit_critical();

        btstack_event_dispatch(&item);
    }
}

int btstack_event_init(void)
{
    btstack_event_reset();
    return BTSTACK_STATUS_OK;
}

void btstack_event_reset(void)
{
    btstack_platform_enter_critical();
    if (btstack_event_is_processing) {
        btstack_platform_exit_critical();
        return;
    }
    memset(btstack_event_queue, 0, sizeof(btstack_event_queue));
    btstack_event_read_index = 0u;
    btstack_event_write_index = 0u;
    btstack_event_count = 0u;
    btstack_app_callback = NULL;
    btstack_hid_get_report_callback = NULL;
    btstack_ble_gatt_write_callback = NULL;
    btstack_audio_sample_callback = NULL;
    btstack_media_data_callback = NULL;
    btstack_hfp_audio_sample_callback = NULL;
    btstack_hfp_callback = NULL;
    btstack_platform_exit_critical();
}

bool btstack_event_query_hid_get_report(
    btstack_transport_t transport,
    uint8_t report_type,
    uint8_t report_id,
    uint8_t *buffer,
    uint16_t *buffer_len)
{
    btstack_hid_get_report_callback_t callback;

    if (buffer_len == NULL) {
        return false;
    }

    if ((buffer == NULL) && (*buffer_len > 0u)) {
        return false;
    }

    btstack_platform_enter_critical();
    callback = btstack_hid_get_report_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return false;
    }

    return callback(transport, report_type, report_id, buffer, buffer_len);
}

bool btstack_event_notify_ble_gatt_write(
    uint16_t value_handle,
    const uint8_t *data,
    uint16_t data_len)
{
    btstack_ble_gatt_write_callback_t callback;
    btstack_queued_event_t item;

    if ((data == NULL) && (data_len > 0u)) {
        return false;
    }

    if (data_len > (uint16_t)BTSTACK_EVENT_DATA_MAX) {
        return false;
    }

    btstack_platform_enter_critical();
    callback = btstack_ble_gatt_write_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return false;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_BLE_GATT_WRITE;
    item.meta.gatt_write.value_handle = value_handle;
    item.data_len = data_len;
    if (data_len > 0u) {
        memcpy(item.data, data, data_len);
    }

    return btstack_event_enqueue(&item) == BTSTACK_STATUS_OK;
}

void btstack_event_notify_a2dp_audio_sample(
    const btstack_audio_sample_t *sample)
{
    btstack_audio_sample_callback_t callback;
    btstack_queued_event_t item;

    if ((sample == NULL) ||
        ((sample->data == NULL) && (sample->data_len > 0u)) ||
        (sample->data_len > (uint16_t)BTSTACK_EVENT_DATA_MAX)) {
        return;
    }

    btstack_platform_enter_critical();
    callback = btstack_audio_sample_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_A2DP_AUDIO_SAMPLE;
    item.meta.audio_sample = *sample;
    item.data_len = sample->data_len;
    if (sample->data_len > 0u) {
        memcpy(item.data, sample->data, sample->data_len);
    }

    (void)btstack_event_enqueue(&item);
}

void btstack_event_notify_a2dp_media_data(
    const btstack_media_packet_t *packet)
{
    btstack_media_data_callback_t callback;
    btstack_queued_event_t item;

    if ((packet == NULL) ||
        ((packet->data == NULL) && (packet->data_len > 0u)) ||
        (packet->data_len > (uint16_t)BTSTACK_EVENT_DATA_MAX)) {
        return;
    }

    btstack_platform_enter_critical();
    callback = btstack_media_data_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_A2DP_MEDIA_DATA;
    item.meta.media_packet = *packet;
    item.data_len = packet->data_len;
    if (packet->data_len > 0u) {
        memcpy(item.data, packet->data, packet->data_len);
    }

    (void)btstack_event_enqueue(&item);
}

void btstack_event_notify_hfp(
    const btstack_hfp_event_t *event)
{
    btstack_hfp_event_callback_t callback;
    btstack_queued_event_t item;

    if (event == NULL) {
        return;
    }

    btstack_platform_enter_critical();
    callback = btstack_hfp_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_HFP_EVENT;
    item.meta.hfp_event = *event;
    item.data_len = btstack_event_copy_text(item.data, event->text);

    (void)btstack_event_enqueue(&item);
}

void btstack_event_notify_hfp_audio_sample(
    const btstack_audio_sample_t *sample)
{
    btstack_audio_sample_callback_t callback;
    btstack_queued_event_t item;

    if ((sample == NULL) ||
        ((sample->data == NULL) && (sample->data_len > 0u)) ||
        (sample->data_len > (uint16_t)BTSTACK_EVENT_DATA_MAX)) {
        return;
    }

    btstack_platform_enter_critical();
    callback = btstack_hfp_audio_sample_callback;
    btstack_platform_exit_critical();

    if (callback == NULL) {
        return;
    }

    memset(&item, 0, sizeof(item));
    item.kind = BTSTACK_QUEUED_HFP_AUDIO_SAMPLE;
    item.meta.audio_sample = *sample;
    item.data_len = sample->data_len;
    if (sample->data_len > 0u) {
        memcpy(item.data, sample->data, sample->data_len);
    }

    (void)btstack_event_enqueue(&item);
}
