#ifndef BT_XSTACK_GATT_H
#define BT_XSTACK_GATT_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_XSTACK_GATT_VALUE_MAX 244u
#define BT_XSTACK_GATT_PROPERTY_READ                    0x0002u
#define BT_XSTACK_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE  0x0004u
#define BT_XSTACK_GATT_PROPERTY_WRITE                   0x0008u
#define BT_XSTACK_GATT_PROPERTY_NOTIFY                  0x0010u
#define BT_XSTACK_GATT_PROPERTY_INDICATE                0x0020u

typedef enum {
    BT_XSTACK_GATT_SECURITY_NONE = 0,
    BT_XSTACK_GATT_SECURITY_ENCRYPTED,
    BT_XSTACK_GATT_SECURITY_AUTHENTICATED
} bt_xstack_gatt_security_t;

typedef struct {
    bt_xstack_uuid_t uuid;
    uint16_t properties;
    uint16_t max_len;
    bt_xstack_gatt_security_t read_security;
    bt_xstack_gatt_security_t write_security;
} bt_xstack_gatt_attribute_config_t;

typedef struct {
    uint16_t start_handle;
    uint16_t end_handle;
    bt_xstack_uuid_t uuid;
} bt_xstack_gatt_service_t;

typedef struct {
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    bt_xstack_uuid_t uuid;
} bt_xstack_gatt_characteristic_t;

typedef struct {
    uint16_t handle;
    bt_xstack_uuid_t uuid;
} bt_xstack_gatt_descriptor_t;

typedef enum {
    BT_XSTACK_GATT_EVENT_SERVICE = 0,
    BT_XSTACK_GATT_EVENT_CHARACTERISTIC,
    BT_XSTACK_GATT_EVENT_DESCRIPTOR,
    BT_XSTACK_GATT_EVENT_VALUE,
    BT_XSTACK_GATT_EVENT_NOTIFICATION,
    BT_XSTACK_GATT_EVENT_INDICATION,
    BT_XSTACK_GATT_EVENT_MTU,
    BT_XSTACK_GATT_EVENT_CLIENT_COMPLETE,
    BT_XSTACK_GATT_EVENT_SERVER_COMPLETE,
    BT_XSTACK_GATT_EVENT_SERVER_VALUE,
    BT_XSTACK_GATT_EVENT_SERVER_WRITE,
    BT_XSTACK_GATT_EVENT_SUBSCRIPTION_CHANGED,
    BT_XSTACK_GATT_EVENT_INDICATION_COMPLETE
} bt_xstack_gatt_event_type_t;

typedef struct {
    uint16_t connection_handle;
    uint16_t value_handle;
    uint16_t value_len;
    uint16_t mtu;
    uint16_t cccd_value;
    uint8_t operation;
    uint8_t truncated;
    uint8_t value[BT_XSTACK_GATT_VALUE_MAX];
} bt_xstack_gatt_value_event_t;

typedef struct {
    bt_xstack_gatt_event_type_t type;
    int status;
    union {
        bt_xstack_gatt_service_t service;
        bt_xstack_gatt_characteristic_t characteristic;
        bt_xstack_gatt_descriptor_t descriptor;
        bt_xstack_gatt_value_event_t value;
    } data;
} bt_xstack_gatt_event_t;

typedef void (*bt_xstack_gatt_callback_t)(const bt_xstack_gatt_event_t *event, void *context);

bt_xstack_result_t bt_xstack_gatt_server_add_service(const bt_xstack_uuid_t *uuid);
bt_xstack_result_t bt_xstack_gatt_server_add_characteristic(uint16_t service_handle,
    const bt_xstack_gatt_attribute_config_t *config);
bt_xstack_result_t bt_xstack_gatt_server_add_descriptor(uint16_t value_handle,
    const bt_xstack_gatt_attribute_config_t *config);
bt_xstack_result_t bt_xstack_gatt_server_build(void);
bt_xstack_result_t bt_xstack_gatt_server_set_value(uint16_t value_handle,
    const uint8_t *value, uint16_t value_len);
bt_xstack_result_t bt_xstack_gatt_server_get_value(uint16_t value_handle);
bt_xstack_result_t bt_xstack_gatt_server_notify(uint16_t value_handle,
    const uint8_t *value, uint16_t value_len);
bt_xstack_result_t bt_xstack_gatt_server_indicate(uint16_t value_handle,
    const uint8_t *value, uint16_t value_len);

bt_xstack_result_t bt_xstack_gatt_client_discover_services(const bt_xstack_uuid_t *uuid);
bt_xstack_result_t bt_xstack_gatt_client_discover_characteristics(uint16_t start_handle,
    uint16_t end_handle, const bt_xstack_uuid_t *uuid);
bt_xstack_result_t bt_xstack_gatt_client_discover_descriptors(
    const bt_xstack_gatt_characteristic_t *characteristic);
bt_xstack_result_t bt_xstack_gatt_client_read(uint16_t value_handle);
bt_xstack_result_t bt_xstack_gatt_client_write(uint16_t value_handle,
    const uint8_t *value, uint16_t value_len, bool without_response);
bt_xstack_result_t bt_xstack_gatt_client_subscribe(
    const bt_xstack_gatt_characteristic_t *characteristic,
    bool notifications, bool indications);
bt_xstack_result_t bt_xstack_gatt_client_unsubscribe(void);

#ifdef __cplusplus
}
#endif

#endif
