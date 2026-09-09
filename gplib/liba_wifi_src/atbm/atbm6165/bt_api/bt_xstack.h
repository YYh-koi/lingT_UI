#ifndef BT_XSTACK_PUBLIC_H
#define BT_XSTACK_PUBLIC_H

#include "bt_xstack_types.h"
#include "bt_xstack_service.h"
#include "bt_xstack_phone.h"
#include "bt_xstack_audio.h"
#include "bt_xstack_phonebook.h"
#include "bt_xstack_le.h"
#include "bt_xstack_gatt.h"
#include "bt_xstack_cover_art.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bt_xstack_service_callback_t service_callback;
    bt_xstack_connection_callback_t connection_callback;
    bt_xstack_phone_callback_t phone_callback;
    bt_xstack_audio_event_callback_t audio_event_callback;
    bt_xstack_phonebook_callback_t phonebook_callback;
    bt_xstack_le_callback_t le_callback;
    bt_xstack_gatt_callback_t gatt_callback;
    bt_xstack_cover_art_callback_t cover_art_callback;
    bt_xstack_media_audio_callback_t media_audio_callback;
    bt_xstack_call_audio_callback_t call_audio_callback;
    void *context;
    uint32_t task_priority;
    /* Zero selects the fixed 2048-word worker stack; 2048 is also accepted. */
    uint16_t task_stack_words;
} bt_xstack_config_t;

bt_xstack_result_t bt_xstack_init(const bt_xstack_config_t *config);

#ifdef __cplusplus
}
#endif

#endif
