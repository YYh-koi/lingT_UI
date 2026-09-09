#ifndef BTSTACK_PROFILE_AVRCP_COVER_ART_H
#define BTSTACK_PROFILE_AVRCP_COVER_ART_H

#include <stdint.h>
#include <stdbool.h>

#include "adapter/btstack_types.h"
#include "core/bt_xstack_cover_art.h"
#include "profiles/btstack_bip_client.h"

#ifdef __cplusplus
extern "C" {
#endif

int btstack_avrcp_cover_art_setup(btstack_bip_event_callback_t callback);
void btstack_avrcp_cover_art_set_callback_enabled(bool enabled);
int btstack_avrcp_cover_art_set_mode(bt_xstack_cover_art_mode_t mode);
int btstack_avrcp_cover_art_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor);
void btstack_avrcp_cover_art_connected(const btstack_bd_addr_t address);
void btstack_avrcp_cover_art_track_changed(void);
void btstack_avrcp_cover_art_handle_received(const uint8_t *value, uint16_t length);
void btstack_avrcp_cover_art_disconnected(const btstack_bd_addr_t address);

#ifdef BTSTACK_AVRCP_COVER_ART_TESTING
void btstack_avrcp_cover_art_test_set_generation(uint32_t generation);
#endif

#ifdef __cplusplus
}
#endif

#endif
