#include <cstddef>

#include "bt_xstack.h"

static void cover_art_callback(const bt_xstack_cover_art_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

int main()
{
    bt_xstack_cover_art_callback_t callback = cover_art_callback;
    bt_xstack_cover_art_image_descriptor_t descriptor;
    descriptor.encoding = BT_XSTACK_COVER_ART_ENCODING_PNG;
    descriptor.width = 320u;
    descriptor.height = 240u;
    descriptor.transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_CROP;
#ifdef BTSTACK_COVER_ART_AT_HEADER
    (void)descriptor;
    return bt_xstack_cover_art_register_callback(callback, NULL);
#else
    (void)callback;
    return static_cast<int>(bt_xstack_cover_art_get_image(&descriptor));
#endif
}
