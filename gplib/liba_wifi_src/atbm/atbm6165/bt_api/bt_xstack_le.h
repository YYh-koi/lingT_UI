#ifndef BT_XSTACK_LE_H
#define BT_XSTACK_LE_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_XSTACK_LE_ADV_DATA_MAX 31u

typedef enum {
    BT_XSTACK_LE_ROLE_NONE = 0,
    BT_XSTACK_LE_ROLE_CENTRAL,
    BT_XSTACK_LE_ROLE_PERIPHERAL
} bt_xstack_le_role_t;

typedef struct {
    uint16_t interval_min;
    uint16_t interval_max;
    uint8_t advertising_type;
    uint8_t own_address_type;
    uint8_t channel_map;
    uint8_t filter_policy;
    uint8_t data_len;
    uint8_t data[BT_XSTACK_LE_ADV_DATA_MAX];
    uint8_t scan_response_len;
    uint8_t scan_response[BT_XSTACK_LE_ADV_DATA_MAX];
} bt_xstack_le_adv_config_t;

typedef struct {
    uint16_t interval;
    uint16_t window;
    bool active;
    bool duplicate_filter;
} bt_xstack_le_scan_config_t;

typedef enum {
    BT_XSTACK_LE_EVENT_ADV_STARTED = 0,
    BT_XSTACK_LE_EVENT_ADV_STOPPED,
    BT_XSTACK_LE_EVENT_SCAN_RESULT,
    BT_XSTACK_LE_EVENT_SCAN_STOPPED,
    BT_XSTACK_LE_EVENT_CONNECTING,
    BT_XSTACK_LE_EVENT_CONNECTED,
    BT_XSTACK_LE_EVENT_DISCONNECTED,
    BT_XSTACK_LE_EVENT_CONNECTION_UPDATED,
    BT_XSTACK_LE_EVENT_PAIRING_REQUEST,
    BT_XSTACK_LE_EVENT_PASSKEY_DISPLAY,
    BT_XSTACK_LE_EVENT_PASSKEY_INPUT,
    BT_XSTACK_LE_EVENT_PAIRING_COMPLETE,
    BT_XSTACK_LE_EVENT_ENCRYPTION_CHANGED
} bt_xstack_le_event_type_t;

typedef struct {
    bt_xstack_addr_t address;
    int8_t rssi;
    uint8_t address_type;
    uint8_t advertising_event_type;
    uint8_t data_len;
    uint8_t data[BT_XSTACK_LE_ADV_DATA_MAX];
} bt_xstack_le_scan_result_t;

typedef struct {
    bt_xstack_addr_t address;
    uint16_t connection_handle;
    uint16_t interval;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint8_t address_type;
    uint8_t reason;
    bt_xstack_le_role_t role;
} bt_xstack_le_link_event_t;

typedef struct {
    uint32_t passkey;
    uint16_t bond_index;
    bool encrypted;
    bool bonded;
} bt_xstack_le_security_event_t;

typedef struct {
    bt_xstack_le_event_type_t type;
    int status;
    union {
        bt_xstack_le_scan_result_t scan;
        bt_xstack_le_link_event_t link;
        bt_xstack_le_security_event_t security;
    } data;
} bt_xstack_le_event_t;

typedef void (*bt_xstack_le_callback_t)(const bt_xstack_le_event_t *event, void *context);

bt_xstack_result_t bt_xstack_le_adv_configure(const bt_xstack_le_adv_config_t *config);
bt_xstack_result_t bt_xstack_le_adv_start(void);
bt_xstack_result_t bt_xstack_le_adv_stop(void);
bt_xstack_result_t bt_xstack_le_scan_start(const bt_xstack_le_scan_config_t *config);
bt_xstack_result_t bt_xstack_le_scan_stop(void);
bt_xstack_result_t bt_xstack_le_connect(uint8_t address_type, const bt_xstack_addr_t *address);
bt_xstack_result_t bt_xstack_le_connect_cancel(void);
bt_xstack_result_t bt_xstack_le_disconnect(void);
bt_xstack_result_t bt_xstack_le_pair(void);
bt_xstack_result_t bt_xstack_le_pairing_confirm(bool accept);
bt_xstack_result_t bt_xstack_le_passkey_reply(uint32_t passkey);
bt_xstack_result_t bt_xstack_le_bond_delete(uint16_t index);
bt_xstack_result_t bt_xstack_le_bond_delete_all(void);

#ifdef __cplusplus
}
#endif

#endif
