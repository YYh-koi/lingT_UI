#ifndef BTSTACK_PROFILE_BIP_CLIENT_H
#define BTSTACK_PROFILE_BIP_CLIENT_H

#include <stdint.h>

#include "adapter/btstack_types.h"
#include "core/bt_xstack_cover_art.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTSTACK_BIP_OPERATION_NONE = 0,
    BTSTACK_BIP_OPERATION_LINKED_THUMBNAIL,
    BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES,
    BTSTACK_BIP_OPERATION_IMAGE
} btstack_bip_operation_t;

typedef enum {
    BTSTACK_BIP_EVENT_STARTED = 0,
    BTSTACK_BIP_EVENT_PROPERTY,
    BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED,
    BTSTACK_BIP_EVENT_DATA,
    BTSTACK_BIP_EVENT_COMPLETED,
    BTSTACK_BIP_EVENT_ERROR,
    BTSTACK_BIP_EVENT_CONNECTION_ESTABLISHED
} btstack_bip_event_type_t;

typedef struct {
    btstack_bip_event_type_t type;
    btstack_bip_operation_t operation;
    int status;
    uint8_t native_status;
    uint32_t generation;
    union {
        struct {
            btstack_bd_addr_t address;
            char image_handle[8];
        } started;
        bt_xstack_cover_art_property_t property;
        struct { uint16_t property_count; } properties_completed;
        struct { const uint8_t *data; uint16_t length; uint32_t offset; } data;
        struct { uint32_t total_length; } completed;
    } data;
} btstack_bip_event_t;

typedef void (*btstack_bip_event_callback_t)(const btstack_bip_event_t *event);

int btstack_bip_client_init(btstack_bip_event_callback_t callback);
int btstack_bip_client_connect(const btstack_bd_addr_t address);
int btstack_bip_client_set_object(const btstack_bd_addr_t address, const char image_handle[8], uint32_t generation);
int btstack_bip_client_get_linked_thumbnail(void);
int btstack_bip_client_get_image_properties(void);
int btstack_bip_client_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor);
int btstack_bip_client_cancel(void);
void btstack_bip_client_peer_disconnected(const btstack_bd_addr_t address);

#ifdef BTSTACK_BIP_CLIENT_TESTING
void btstack_bip_client_test_properties_reset(uint32_t generation);
int btstack_bip_client_test_properties_feed(const uint8_t *data, uint16_t length, uint8_t final_chunk);
#endif

#ifdef __cplusplus
}
#endif

#endif
