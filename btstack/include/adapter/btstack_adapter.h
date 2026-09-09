#ifndef BTSTACK_ADAPTER_H
#define BTSTACK_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initializes and resets common property, event, GAP, and bond state.
 * Controller and profile lifecycle is owned by the runtime and profile
 * manager, not by this module.
 */
typedef enum {
    BTSTACK_ADAPTER_EVENT_STACK_STATE = 0,
    BTSTACK_ADAPTER_EVENT_LOCAL_ADDRESS,
    BTSTACK_ADAPTER_EVENT_ACL_CONNECTED,
    BTSTACK_ADAPTER_EVENT_ACL_DISCONNECTED,
    BTSTACK_ADAPTER_EVENT_SCAN_RESULT,
    BTSTACK_ADAPTER_EVENT_SCAN_COMPLETE,
    BTSTACK_ADAPTER_EVENT_PAIRING_REQUEST,
    BTSTACK_ADAPTER_EVENT_PAIRING_COMPLETE,
    BTSTACK_ADAPTER_EVENT_LE
} btstack_adapter_event_type_t;

typedef enum {
    BTSTACK_ADAPTER_LE_EVENT_SCAN_RESULT = 0,
    BTSTACK_ADAPTER_LE_EVENT_CONNECTED,
    BTSTACK_ADAPTER_LE_EVENT_DISCONNECTED,
    BTSTACK_ADAPTER_LE_EVENT_CONNECTION_UPDATED,
    BTSTACK_ADAPTER_LE_EVENT_JUST_WORKS_REQUEST,
    BTSTACK_ADAPTER_LE_EVENT_NUMERIC_COMPARISON,
    BTSTACK_ADAPTER_LE_EVENT_PASSKEY_DISPLAY,
    BTSTACK_ADAPTER_LE_EVENT_PASSKEY_INPUT,
    BTSTACK_ADAPTER_LE_EVENT_PAIRING_COMPLETE,
    BTSTACK_ADAPTER_LE_EVENT_IDENTITY_RESOLVED,
    BTSTACK_ADAPTER_LE_EVENT_REENCRYPTION_COMPLETE
} btstack_adapter_le_event_type_t;

typedef enum {
    BTSTACK_ADAPTER_LE_ROLE_NONE = 0,
    BTSTACK_ADAPTER_LE_ROLE_CENTRAL,
    BTSTACK_ADAPTER_LE_ROLE_PERIPHERAL
} btstack_adapter_le_role_t;

typedef struct {
    uint16_t interval_min;
    uint16_t interval_max;
    uint8_t advertising_type;
    uint8_t own_address_type;
    uint8_t channel_map;
    uint8_t filter_policy;
    uint8_t data_len;
    uint8_t data[BTSTACK_ADV_DATA_MAX];
    uint8_t scan_response_len;
    uint8_t scan_response[BTSTACK_ADV_DATA_MAX];
} btstack_adapter_le_adv_config_t;

typedef struct {
    uint16_t interval;
    uint16_t window;
    uint8_t active;
    uint8_t duplicate_filter;
} btstack_adapter_le_scan_config_t;

typedef struct {
    btstack_adapter_le_event_type_t type;
    int status;
    btstack_bd_addr_t address;
    btstack_adapter_le_role_t role;
    uint16_t connection_handle;
    uint16_t connection_interval;
    uint16_t connection_latency;
    uint16_t supervision_timeout;
    uint32_t passkey;
    uint16_t bond_index;
    uint8_t address_type;
    uint8_t advertising_event_type;
    uint8_t reason;
    int8_t rssi;
    uint8_t data_len;
    uint8_t data[BTSTACK_ADV_DATA_MAX];
} btstack_adapter_le_event_t;

typedef struct {
    btstack_adapter_event_type_t type;
    union {
        struct {
            bool working;
        } stack_state;
        struct {
            btstack_bd_addr_t address;
        } address;
        struct {
            btstack_bd_addr_t address;
            uint32_t class_of_device;
            int8_t rssi;
            uint8_t name_length;
            char name[BTSTACK_LOCAL_NAME_MAX];
        } scan_result;
        struct {
            btstack_bd_addr_t address;
            uint32_t numeric_value;
        } pairing_request;
        struct {
            btstack_bd_addr_t address;
            int status;
        } pairing_complete;
        btstack_adapter_le_event_t le;
    } data;
} btstack_adapter_event_t;

typedef struct {
    btstack_bd_addr_t local_address;
    btstack_bd_addr_t peer_address;
    char local_name[BTSTACK_LOCAL_NAME_MAX];
    char pin[17];
    uint32_t class_of_device;
    uint8_t ssp_mode;
    bool stack_working;
    bool connectable;
    bool discoverable;
    bool scanning;
    bool acl_connected;
} btstack_adapter_state_t;

typedef void (*btstack_adapter_event_callback_t)(
    const btstack_adapter_event_t *event);

int btstack_adapter_setup(void);
void btstack_adapter_deinit(void);
int btstack_adapter_set_event_callback(
    btstack_adapter_event_callback_t callback);
int btstack_adapter_set_power(bool enabled);
int btstack_adapter_set_name(const char *name);
int btstack_adapter_set_name_with_address_suffix(const char *name);
int btstack_adapter_set_pin(const char *pin);
int btstack_adapter_set_ssp_mode(uint8_t mode);
int btstack_adapter_pairing_confirm(
    const btstack_bd_addr_t address,
    bool accept);
int btstack_adapter_set_discoverable(bool enabled);
int btstack_adapter_set_connectable(bool enabled);
int btstack_adapter_set_class_of_device(uint32_t class_of_device);
int btstack_adapter_scan_start(uint8_t duration);
int btstack_adapter_scan_stop(void);
int btstack_adapter_disconnect(void);
int btstack_adapter_le_adv_configure(const btstack_adapter_le_adv_config_t *config);
int btstack_adapter_le_adv_start(void);
int btstack_adapter_le_adv_stop(void);
int btstack_adapter_le_scan_start(const btstack_adapter_le_scan_config_t *config);
int btstack_adapter_le_scan_stop(void);
int btstack_adapter_le_connect(uint8_t address_type, const btstack_bd_addr_t address);
int btstack_adapter_le_connect_cancel(void);
int btstack_adapter_le_disconnect(void);
int btstack_adapter_le_pair(void);
int btstack_adapter_le_pairing_confirm(uint16_t connection_handle, bool accept);
int btstack_adapter_le_passkey_reply(uint16_t connection_handle, uint32_t passkey);
int btstack_adapter_le_bond_delete(uint8_t index);
int btstack_adapter_le_bond_delete_all(void);
bool btstack_adapter_is_working(void);
int btstack_adapter_get_local_address(btstack_bd_addr_t address);
int btstack_adapter_get_state(btstack_adapter_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
