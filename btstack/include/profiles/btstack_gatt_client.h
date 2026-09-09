#ifndef BTSTACK_GATT_CLIENT_PROFILE_H
#define BTSTACK_GATT_CLIENT_PROFILE_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"
#include "ble/gatt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTSTACK_GATT_CLIENT_OPERATION_NONE = 0,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_SERVICES,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_CHARACTERISTICS,
    BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_DESCRIPTORS,
    BTSTACK_GATT_CLIENT_OPERATION_READ,
    BTSTACK_GATT_CLIENT_OPERATION_WRITE,
    BTSTACK_GATT_CLIENT_OPERATION_SUBSCRIBE,
    BTSTACK_GATT_CLIENT_OPERATION_UNSUBSCRIBE
} btstack_gatt_client_operation_t;

typedef enum {
    BTSTACK_GATT_CLIENT_EVENT_SERVICE = 0,
    BTSTACK_GATT_CLIENT_EVENT_CHARACTERISTIC,
    BTSTACK_GATT_CLIENT_EVENT_DESCRIPTOR,
    BTSTACK_GATT_CLIENT_EVENT_VALUE,
    BTSTACK_GATT_CLIENT_EVENT_NOTIFICATION,
    BTSTACK_GATT_CLIENT_EVENT_INDICATION,
    BTSTACK_GATT_CLIENT_EVENT_MTU,
    BTSTACK_GATT_CLIENT_EVENT_COMPLETE
} btstack_gatt_client_event_type_t;

typedef struct {
    btstack_gatt_client_event_type_t type;
    btstack_gatt_client_operation_t operation;
    int status;
    uint16_t connection_handle;
    uint16_t value_handle;
    uint16_t value_len;
    uint16_t mtu;
    uint8_t truncated;
    union {
        gatt_client_service_t service;
        gatt_client_characteristic_t characteristic;
        gatt_client_characteristic_descriptor_t descriptor;
        uint8_t value[BTSTACK_GATT_VALUE_MAX];
    } data;
} btstack_gatt_client_event_t;

typedef void (*btstack_gatt_client_event_callback_t)(const btstack_gatt_client_event_t *event);

int btstack_gatt_client_init(void);
void btstack_gatt_client_reset(void);
int btstack_gatt_client_set_event_callback(btstack_gatt_client_event_callback_t callback);
void btstack_gatt_client_connection_changed(uint16_t connection_handle, bool connected);
btstack_gatt_client_operation_t btstack_gatt_client_get_operation(void);
int btstack_gatt_client_discover_services(const btstack_uuid_t *uuid);
int btstack_gatt_client_discover_characteristics(uint16_t start_handle, uint16_t end_handle,
    const btstack_uuid_t *uuid);
int btstack_gatt_client_discover_descriptors(const gatt_client_characteristic_t *characteristic);
int btstack_gatt_client_read(uint16_t value_handle);
int btstack_gatt_client_write(uint16_t value_handle, const uint8_t *value, uint16_t value_len,
    bool without_response);
int btstack_gatt_client_subscribe(const gatt_client_characteristic_t *characteristic,
    bool notifications, bool indications);
int btstack_gatt_client_unsubscribe(void);

#ifdef __cplusplus
}
#endif

#endif
