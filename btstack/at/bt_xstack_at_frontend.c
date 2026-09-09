#include "../include/at/bt_xstack.h"

#include "../include/at/btstack_at.h"
#include "../core/btstack_runtime.h"
#include "../include/port/btstack_platform.h"
#include "../include/profiles/btstack_avrcp_cover_art.h"

int btstack_at_core_adapter_init(void);
int btstack_at_set_pcm_callback(
    bt_xstack_media_audio_callback_t callback);

static int bt_xstack_at_initialized;
static bt_event_callback_t bt_xstack_at_callback;
static void *bt_xstack_at_context;
static bt_xstack_cover_art_callback_t bt_xstack_at_cover_art_callback;
static void *bt_xstack_at_cover_art_context;

int bt_xstack_init(bt_event_callback_t event_callback, void *context)
{
    btstack_at_config_t config;
    int status;

    if (event_callback == NULL) return BT_XSTACK_STATUS_INVALID_ARGUMENT;
    if (bt_xstack_at_initialized) {
        return ((bt_xstack_at_callback == event_callback) &&
            (bt_xstack_at_context == context)) ? BT_XSTACK_STATUS_OK :
            BT_XSTACK_STATUS_CONFIGURATION_CONFLICT;
    }
    config.output_callback = event_callback;
    config.output_context = context;
    config.task_priority = 0u;
    config.task_stack_size = 0u;
    status = btstack_runtime_init();
    if (status != 0) return BT_XSTACK_STATUS_INTERNAL_ERROR;
    status = btstack_at_init(&config);
    if (status != BTSTACK_AT_STATUS_OK) return status;
    status = btstack_at_core_adapter_init();
    if (status != BTSTACK_AT_STATUS_OK) return status;
    btstack_avrcp_cover_art_set_callback_enabled(
        bt_xstack_at_cover_art_callback != NULL);
    status = btstack_at_start();
    if (status != BTSTACK_AT_STATUS_OK) return status;
    bt_xstack_at_callback = event_callback;
    bt_xstack_at_context = context;
    bt_xstack_at_initialized = 1;
    return BT_XSTACK_STATUS_OK;
}

int bt_xstack_send_at(const char *command, uint16_t length)
{
    if (!bt_xstack_at_initialized) return BT_XSTACK_STATUS_NOT_INITIALIZED;
    return btstack_at_submit(command, length);
}

int bt_xstack_a2dp_register_pcm_callback(
    bt_xstack_media_audio_callback_t callback)
{
    return btstack_at_set_pcm_callback(callback);
}

int bt_xstack_cover_art_register_callback(
    bt_xstack_cover_art_callback_t callback, void *context)
{
    btstack_platform_enter_critical();
    bt_xstack_at_cover_art_callback = callback;
    bt_xstack_at_cover_art_context = context;
    btstack_platform_exit_critical();
    if (bt_xstack_at_initialized) {
        btstack_avrcp_cover_art_set_callback_enabled(callback != NULL);
    }
    return BT_XSTACK_STATUS_OK;
}

void btstack_frontend_deliver_cover_art(
    const bt_xstack_cover_art_event_t *event)
{
    bt_xstack_cover_art_callback_t callback;
    void *context;

    if (event == NULL) return;
    btstack_platform_enter_critical();
    callback = bt_xstack_at_cover_art_callback;
    context = bt_xstack_at_cover_art_context;
    btstack_platform_exit_critical();
    if (callback != NULL) callback(event, context);
}
