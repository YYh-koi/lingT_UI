#include <stddef.h>
#include <stdint.h>

#include "bt_xstack.h"

typedef char descriptor_must_remain_bounded[
    sizeof(bt_xstack_cover_art_image_descriptor_t) <= 16u ? 1 : -1];
typedef char property_must_remain_bounded[
    sizeof(bt_xstack_cover_art_property_t) <= 32u ? 1 : -1];

static void cover_art_callback(const bt_xstack_cover_art_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

int main(void)
{
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG,
        200u,
        200u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_NONE
    };
    bt_xstack_cover_art_event_t event;
    bt_xstack_cover_art_callback_t callback = cover_art_callback;

    event.type = BT_XSTACK_COVER_ART_DATA;
    event.status = 0;
    event.generation = 1u;
    event.data.data.data = NULL;
    event.data.data.length = 0u;
    event.data.data.offset = 0u;
    callback(&event, NULL);

    event.type = BT_XSTACK_COVER_ART_STARTED;
    event.data.started.address[0] = 0u;
    event.data.started.image_handle[0] = '\0';
    event.data.started.operation = 0u;
    event.type = BT_XSTACK_COVER_ART_PROPERTY;
    event.data.property.encoding = BT_XSTACK_COVER_ART_ENCODING_GIF;
    event.data.property.min_width = 1u;
    event.data.property.max_width = 2u;
    event.data.property.min_height = 3u;
    event.data.property.max_height = 4u;
    event.data.property.file_size = 5u;
    event.data.property.file_size_known = 1u;
    event.data.property.native_image = 1u;
    event.data.property.transformations = BT_XSTACK_COVER_ART_TRANSFORMATION_ALL;
    event.type = BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED;
    event.data.properties_completed.property_count = 1u;
    event.type = BT_XSTACK_COVER_ART_COMPLETED;
    event.data.completed.total_length = 6u;
    event.type = BT_XSTACK_COVER_ART_ERROR;
    event.data.error.operation = 1u;
    event.data.error.native_status = 2u;
    callback(&event, NULL);

#ifdef BTSTACK_COVER_ART_AT_HEADER
    (void)descriptor;
    return bt_xstack_cover_art_register_callback(callback, NULL);
#else
    if (bt_xstack_cover_art_set_mode(
            BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL) != BT_XSTACK_OK) {
        return 1;
    }
    return (int)bt_xstack_cover_art_get_image(&descriptor);
#endif
}
