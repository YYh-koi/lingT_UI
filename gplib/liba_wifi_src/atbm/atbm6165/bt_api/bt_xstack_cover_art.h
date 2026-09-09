#ifndef BT_XSTACK_COVER_ART_H
#define BT_XSTACK_COVER_ART_H

#include <stdint.h>

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL = 0,
    BT_XSTACK_COVER_ART_MODE_IMAGE
} bt_xstack_cover_art_mode_t;

typedef enum {
    BT_XSTACK_COVER_ART_ENCODING_UNKNOWN = 0,
    BT_XSTACK_COVER_ART_ENCODING_JPEG,
    BT_XSTACK_COVER_ART_ENCODING_PNG,
    BT_XSTACK_COVER_ART_ENCODING_GIF,
    BT_XSTACK_COVER_ART_ENCODING_BMP,
    BT_XSTACK_COVER_ART_ENCODING_WBMP
} bt_xstack_cover_art_encoding_t;

typedef enum {
    BT_XSTACK_COVER_ART_TRANSFORMATION_NONE = 0,
    BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH = 1u << 0,
    BT_XSTACK_COVER_ART_TRANSFORMATION_FILL = 1u << 1,
    BT_XSTACK_COVER_ART_TRANSFORMATION_CROP = 1u << 2,
    BT_XSTACK_COVER_ART_TRANSFORMATION_ALL =
        BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH |
        BT_XSTACK_COVER_ART_TRANSFORMATION_FILL |
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP
} bt_xstack_cover_art_transformation_t;

typedef struct {
    bt_xstack_cover_art_encoding_t encoding;
    uint16_t width;
    uint16_t height;
    uint8_t transformation;
} bt_xstack_cover_art_image_descriptor_t;

typedef struct {
    bt_xstack_cover_art_encoding_t encoding;
    uint16_t min_width;
    uint16_t max_width;
    uint16_t min_height;
    uint16_t max_height;
    uint32_t file_size;
    uint8_t file_size_known;
    uint8_t native_image;
    uint8_t transformations;
} bt_xstack_cover_art_property_t;

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
            uint8_t address[BT_XSTACK_ADDRESS_LEN];
            char image_handle[8];
            uint8_t operation;
        } started;
        bt_xstack_cover_art_property_t property;
        struct {
            uint16_t property_count;
        } properties_completed;
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
bt_xstack_result_t bt_xstack_cover_art_set_mode(
    bt_xstack_cover_art_mode_t mode);
bt_xstack_result_t bt_xstack_cover_art_get_image(
    const bt_xstack_cover_art_image_descriptor_t *descriptor);

#ifdef __cplusplus
}
#endif

#endif
