#ifndef BTSTACK_ADAPTER_EVENT_H
#define BTSTACK_ADAPTER_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

/* The adapter public API requires C99-or-newer C, or C++ with these headers. */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*btstack_app_event_callback_t)(
    btstack_transport_t transport,
    btstack_event_t event,
    const uint8_t *data,
    uint16_t data_len);

/*
 * Event payload pointers are only valid for the duration of the callback.
 * btstack_event_post() and btstack_event_post_hid_report()
 * copy payload data before queueing and reject oversized payloads.
 * notify APIs copy bounded payloads before queueing when the callback has
 * an asynchronous return path. Query APIs remain synchronous because the
 * protocol stack needs an immediate answer.
 * Asynchronous ports should signal or schedule an event worker, then call
 * btstack_event_process() from that worker.
 */

typedef bool (*btstack_hid_get_report_callback_t)(
    btstack_transport_t transport,
    uint8_t report_type,
    uint8_t report_id,
    uint8_t *buffer,
    uint16_t *buffer_len);

typedef bool (*btstack_ble_gatt_write_callback_t)(
    uint16_t value_handle,
    const uint8_t *data,
    uint16_t data_len);

typedef void (*btstack_audio_sample_callback_t)(
    const btstack_audio_sample_t *sample);

typedef void (*btstack_media_data_callback_t)(
    const btstack_media_packet_t *packet);

typedef void (*btstack_hfp_event_callback_t)(
    const btstack_hfp_event_t *event);

int btstack_event_init(void);
void btstack_event_register_app_callback(
    btstack_app_event_callback_t callback);
void btstack_event_register_hid_get_report_callback(
    btstack_hid_get_report_callback_t callback);
void btstack_event_register_ble_gatt_write_callback(
    btstack_ble_gatt_write_callback_t callback);
void btstack_event_register_a2dp_audio_sample_callback(
    btstack_audio_sample_callback_t callback);
void btstack_event_register_a2dp_media_data_callback(
    btstack_media_data_callback_t callback);
void btstack_event_register_hfp_audio_sample_callback(
    btstack_audio_sample_callback_t callback);
void btstack_event_register_hfp_callback(
    btstack_hfp_event_callback_t callback);

int btstack_event_post(
    btstack_transport_t transport,
    btstack_event_t event,
    const uint8_t *data,
    uint16_t data_len);

int btstack_event_post_hid_report(
    btstack_transport_t transport,
    uint8_t report_type,
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len);

void btstack_event_process(void);
/* reset() is quiescent-only; calls made during dispatch are ignored. */
void btstack_event_reset(void);

bool btstack_event_query_hid_get_report(
    btstack_transport_t transport,
    uint8_t report_type,
    uint8_t report_id,
    uint8_t *buffer,
    uint16_t *buffer_len);

bool btstack_event_notify_ble_gatt_write(
    uint16_t value_handle,
    const uint8_t *data,
    uint16_t data_len);

void btstack_event_notify_a2dp_audio_sample(
    const btstack_audio_sample_t *sample);
void btstack_event_notify_a2dp_media_data(
    const btstack_media_packet_t *packet);
void btstack_event_notify_hfp_audio_sample(
    const btstack_audio_sample_t *sample);
void btstack_event_notify_hfp(
    const btstack_hfp_event_t *event);

#ifdef __cplusplus
}
#endif

#endif
