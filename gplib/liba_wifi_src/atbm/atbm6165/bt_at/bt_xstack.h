#ifndef BT_XSTACK_AT_FRONTEND_H
#define BT_XSTACK_AT_FRONTEND_H

#include <stdint.h>

#include "bt_xstack_cover_art.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bt_event_callback_t)(
    const char *data,
    uint16_t length,
    void *context);

typedef void (*bt_xstack_media_audio_callback_t)(
    const uint8_t *data,
    uint32_t length);

typedef enum {
    BT_XSTACK_STATUS_OK = 0,
    BT_XSTACK_STATUS_INVALID_ARGUMENT = -1,
    BT_XSTACK_STATUS_NOT_INITIALIZED = -2,
    BT_XSTACK_STATUS_CONFIGURATION_CONFLICT = -3,
    BT_XSTACK_STATUS_COMMAND_TOO_LONG = -4,
    BT_XSTACK_STATUS_QUEUE_FULL = -5,
    BT_XSTACK_STATUS_NOT_READY = -6,
    BT_XSTACK_STATUS_INTERNAL_ERROR = -7
} bt_xstack_status_t;

int bt_xstack_init(bt_event_callback_t event_callback, void *context);
int bt_xstack_send_at(const char *command, uint16_t length);
int bt_xstack_a2dp_register_pcm_callback(
    bt_xstack_media_audio_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif
