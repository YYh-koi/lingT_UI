#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "at/bt_xstack.h"
#include "at/btstack_at.h"

void btstack_frontend_deliver_cover_art(
    const bt_xstack_cover_art_event_t *event);

static unsigned int critical_depth;
static unsigned int callback_count;
static unsigned int enabled_count;
static unsigned int disabled_count;
static const bt_xstack_cover_art_event_t *delivered_event;
static void *delivered_context;

void btstack_platform_enter_critical(void) { critical_depth++; }
void btstack_platform_exit_critical(void)
{
    assert(critical_depth == 1u);
    critical_depth--;
}

int btstack_runtime_init(void) { return 0; }
int btstack_at_init(const btstack_at_config_t *config)
{
    assert(config != NULL);
    return BTSTACK_AT_STATUS_OK;
}
int btstack_at_core_adapter_init(void) { return BTSTACK_AT_STATUS_OK; }
int btstack_at_start(void) { return BTSTACK_AT_STATUS_OK; }
int btstack_at_submit(const char *command, uint16_t length)
{
    (void)command;
    (void)length;
    return BTSTACK_AT_STATUS_OK;
}
int btstack_at_set_pcm_callback(bt_xstack_media_audio_callback_t callback)
{
    (void)callback;
    return BTSTACK_AT_STATUS_OK;
}
void btstack_avrcp_cover_art_set_callback_enabled(bool enabled)
{
    if (enabled) enabled_count++;
    else disabled_count++;
}

static void text_output(const char *data, uint16_t length, void *context)
{
    (void)data;
    (void)length;
    (void)context;
}

static void cover_art_callback(const bt_xstack_cover_art_event_t *event,
    void *context)
{
    assert(critical_depth == 0u);
    callback_count++;
    delivered_event = event;
    delivered_context = context;
}

int main(void)
{
    bt_xstack_cover_art_event_t event;
    uint8_t bytes[] = { 1u, 2u, 3u };
    uint32_t context = 0x12345678u;

    assert(bt_xstack_cover_art_register_callback(cover_art_callback,
        &context) == BT_XSTACK_STATUS_OK);
    assert(enabled_count == 0u);
    assert(bt_xstack_init(text_output, NULL) == BT_XSTACK_STATUS_OK);
    assert(enabled_count == 1u);

    event.type = BT_XSTACK_COVER_ART_DATA;
    event.status = 0;
    event.generation = 1u;
    event.data.data.data = bytes;
    event.data.data.length = sizeof(bytes);
    event.data.data.offset = 7u;
    btstack_frontend_deliver_cover_art(&event);
    assert(callback_count == 1u);
    assert(delivered_event == &event);
    assert(delivered_event->data.data.data == bytes);
    assert(delivered_context == &context);

    assert(bt_xstack_cover_art_register_callback(NULL, NULL) ==
        BT_XSTACK_STATUS_OK);
    assert(disabled_count == 1u);
    btstack_frontend_deliver_cover_art(&event);
    assert(callback_count == 1u);
    return 0;
}
