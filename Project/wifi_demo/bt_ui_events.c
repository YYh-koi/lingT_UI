#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "bt_ui_events.h"

static bt_ui_notify_cb_t s_ui_notify_cb;
static void *s_ui_notify_ctx;

void bt_ui_set_notify(bt_ui_notify_cb_t callback, void *context)
{
    taskENTER_CRITICAL();
    s_ui_notify_cb = callback;
    s_ui_notify_ctx = context;
    taskEXIT_CRITICAL();
}

void bt_ui_event_init(bt_ui_event_t *event)
{
    if (event == NULL) {
        return;
    }
    memset(event, 0, sizeof(*event));
}

void bt_ui_post_event(const bt_ui_event_t *event)
{
    bt_ui_notify_cb_t cb;
    void *ctx;

    if (event == NULL) {
        return;
    }
    taskENTER_CRITICAL();
    cb = s_ui_notify_cb;
    ctx = s_ui_notify_ctx;
    taskEXIT_CRITICAL();
    if (cb != NULL) {
        cb(event, ctx);
    }
}

void bt_ui_post_simple(
    bt_ui_event_type_t type,
    uint32_t v0, uint32_t v1, uint32_t v2)
{
    bt_ui_event_t event;

    bt_ui_event_init(&event);
    event.type = type;
    event.values[0] = v0;
    event.values[1] = v1;
    event.values[2] = v2;
    event.value_count = 3u;
    bt_ui_post_event(&event);
}

void bt_ui_post_text(
    bt_ui_event_type_t type,
    uint32_t v0, uint32_t v1, uint32_t v2,
    const char *text)
{
    bt_ui_event_t event;

    bt_ui_event_init(&event);
    event.type = type;
    event.values[0] = v0;
    event.values[1] = v1;
    event.values[2] = v2;
    event.value_count = 3u;
    if (text != NULL) {
        strncpy(event.text, text, BT_UI_TEXT_MAX - 1u);
    }
    bt_ui_post_event(&event);
}

void bt_ui_post_track_info(
    const char *title, const char *artist, const char *album)
{
    bt_ui_event_t event;

    bt_ui_event_init(&event);
    event.type = BT_UI_EVENT_TRACK_INFO;
    if (title != NULL) {
        strncpy(event.text, title, BT_UI_TEXT_MAX - 1u);
    }
    if (artist != NULL) {
        strncpy(event.text2, artist, BT_UI_TEXT_MAX - 1u);
    }
    if (album != NULL) {
        strncpy(event.text3, album, BT_UI_TEXT_MAX - 1u);
    }
    bt_ui_post_event(&event);
}
