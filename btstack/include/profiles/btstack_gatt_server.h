#ifndef BTSTACK_GATT_SERVER_H
#define BTSTACK_GATT_SERVER_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_GATT_PROPERTY_BROADCAST              0x0001u
#define BTSTACK_GATT_PROPERTY_READ                   0x0002u
#define BTSTACK_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE 0x0004u
#define BTSTACK_GATT_PROPERTY_WRITE                  0x0008u
#define BTSTACK_GATT_PROPERTY_NOTIFY                 0x0010u
#define BTSTACK_GATT_PROPERTY_INDICATE               0x0020u
#define BTSTACK_GATT_PROPERTY_SIGNED_WRITE           0x0040u
#define BTSTACK_GATT_PROPERTY_EXTENDED               0x0080u
#define BTSTACK_GATT_PROPERTY_DYNAMIC                0x0100u

typedef enum {
    BTSTACK_GATT_SECURITY_NONE = 0,
    BTSTACK_GATT_SECURITY_ENCRYPTED,
    BTSTACK_GATT_SECURITY_AUTHENTICATED
} btstack_gatt_security_t;

typedef struct {
    btstack_uuid_t uuid;
    uint16_t properties;
    uint16_t max_len;
    btstack_gatt_security_t read_security;
    btstack_gatt_security_t write_security;
} btstack_gatt_server_characteristic_config_t;

typedef btstack_gatt_server_characteristic_config_t btstack_gatt_server_descriptor_config_t;

typedef enum {
    BTSTACK_GATT_SERVER_EVENT_VALUE_WRITTEN = 0,
    BTSTACK_GATT_SERVER_EVENT_SUBSCRIPTION_CHANGED,
    BTSTACK_GATT_SERVER_EVENT_INDICATION_COMPLETE
} btstack_gatt_server_event_type_t;

typedef struct {
    btstack_gatt_server_event_type_t type;
    uint16_t connection_handle;
    uint16_t attribute_handle;
    uint16_t value_len;
    uint16_t cccd_value;
    int status;
} btstack_gatt_server_event_t;

typedef void (*btstack_gatt_server_event_callback_t)(const btstack_gatt_server_event_t *event);

int btstack_gatt_server_init(void);
int btstack_gatt_server_reset(void);
int btstack_gatt_server_set_event_callback(btstack_gatt_server_event_callback_t callback);
int btstack_gatt_server_add_service(const btstack_uuid_t *uuid, uint16_t *service_handle);
int btstack_gatt_server_add_characteristic(uint16_t service_handle,
    const btstack_gatt_server_characteristic_config_t *config, uint16_t *value_handle);
int btstack_gatt_server_add_descriptor(uint16_t value_handle,
    const btstack_gatt_server_descriptor_config_t *config, uint16_t *descriptor_handle);
int btstack_gatt_server_build(void);
int btstack_gatt_server_set_value(uint16_t attribute_handle, const uint8_t *value, uint16_t value_len);
int btstack_gatt_server_get_value(uint16_t attribute_handle, uint8_t *value, uint16_t *value_len);
void btstack_gatt_server_connection_changed(uint16_t connection_handle, bool connected);
int btstack_gatt_server_notify(uint16_t value_handle, const uint8_t *value, uint16_t value_len);
int btstack_gatt_server_indicate(uint16_t value_handle, const uint8_t *value, uint16_t value_len);

#ifdef __cplusplus
}
#endif

#endif
