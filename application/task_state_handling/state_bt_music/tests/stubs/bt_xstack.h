#ifndef TEST_BT_XSTACK_H
#define TEST_BT_XSTACK_H

#include <stdint.h>

typedef int32_t bt_xstack_result_t;

#define BT_XSTACK_OK 0

typedef enum {
    BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL = 0,
    BT_XSTACK_COVER_ART_MODE_IMAGE
} bt_xstack_cover_art_mode_t;

typedef enum {
    BT_XSTACK_COVER_ART_STARTED = 0,
    BT_XSTACK_COVER_ART_PROPERTY,
    BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED,
    BT_XSTACK_COVER_ART_DATA,
    BT_XSTACK_COVER_ART_COMPLETED,
    BT_XSTACK_COVER_ART_ERROR
} bt_xstack_cover_art_event_type_t;

typedef struct {
    bt_xstack_cover_art_event_type_t type;
    int32_t status;
    uint32_t generation;
    union {
        struct {
            uint8_t operation;
        } started;
        struct {
            const uint8_t *data;
            uint16_t length;
            uint32_t offset;
        } data;
        struct {
            uint32_t total_length;
        } completed;
        struct {
            uint8_t operation;
            uint8_t native_status;
        } error;
    } data;
} bt_xstack_cover_art_event_t;

typedef void (*bt_xstack_cover_art_callback_t)(
    const bt_xstack_cover_art_event_t *event, void *context);

bt_xstack_result_t bt_xstack_cover_art_register_callback(
    bt_xstack_cover_art_callback_t callback, void *context);
bt_xstack_result_t bt_xstack_cover_art_set_mode(bt_xstack_cover_art_mode_t mode);
bt_xstack_result_t bt_xstack_avrcp_play_pause(void);
bt_xstack_result_t bt_xstack_avrcp_previous(void);
bt_xstack_result_t bt_xstack_avrcp_next(void);

#endif
