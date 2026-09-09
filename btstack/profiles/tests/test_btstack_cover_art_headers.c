#include <stddef.h>
#include <stdint.h>

#include "profiles/btstack_avrcp_cover_art.h"
#include "profiles/btstack_bip_client.h"

static void on_bip_event(const btstack_bip_event_t *event)
{
    (void)event;
}

int main(void)
{
    btstack_bd_addr_t address = { 0u };
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG,
        200u,
        200u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_NONE
    };
    btstack_bip_operation_t operation = BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES;
    btstack_bip_event_callback_t callback = on_bip_event;

    (void)operation;
    (void)callback;
    (void)btstack_bip_client_init(on_bip_event);
    (void)btstack_bip_client_set_object(address, "0123456", 1u);
    (void)btstack_bip_client_get_linked_thumbnail();
    (void)btstack_bip_client_get_image_properties();
    (void)btstack_bip_client_get_image(&descriptor);
    (void)btstack_bip_client_cancel();
    btstack_bip_client_peer_disconnected(address);

    (void)btstack_avrcp_cover_art_setup(on_bip_event);
    (void)btstack_avrcp_cover_art_set_mode(BT_XSTACK_COVER_ART_MODE_IMAGE);
    (void)btstack_avrcp_cover_art_get_image(&descriptor);
    btstack_avrcp_cover_art_connected(address);
    btstack_avrcp_cover_art_track_changed();
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"0123456", 7u);
    btstack_avrcp_cover_art_disconnected(address);
    return 0;
}
