#ifndef BTSTACK_GAP_H
#define BTSTACK_GAP_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_GAP_PIN_MAX 16u

typedef enum {
    BTSTACK_GAP_EVENT_STACK_STATE = 0,
    BTSTACK_GAP_EVENT_LOCAL_ADDRESS,
    BTSTACK_GAP_EVENT_ACL_CONNECTED,
    BTSTACK_GAP_EVENT_ACL_DISCONNECTED,
    BTSTACK_GAP_EVENT_INQUIRY_RESULT,
    BTSTACK_GAP_EVENT_INQUIRY_COMPLETE,
    BTSTACK_GAP_EVENT_PAIRING_REQUEST,
    BTSTACK_GAP_EVENT_PAIRING_COMPLETE,
    BTSTACK_GAP_EVENT_LE_SCAN_RESULT,
    BTSTACK_GAP_EVENT_LE_CONNECTED,
    BTSTACK_GAP_EVENT_LE_DISCONNECTED,
    BTSTACK_GAP_EVENT_LE_CONNECTION_UPDATED,
    BTSTACK_GAP_EVENT_LE_JUST_WORKS_REQUEST,
    BTSTACK_GAP_EVENT_LE_NUMERIC_COMPARISON,
    BTSTACK_GAP_EVENT_LE_PASSKEY_DISPLAY,
    BTSTACK_GAP_EVENT_LE_PASSKEY_INPUT,
    BTSTACK_GAP_EVENT_LE_PAIRING_COMPLETE,
    BTSTACK_GAP_EVENT_LE_IDENTITY_RESOLVED,
    BTSTACK_GAP_EVENT_LE_REENCRYPTION_COMPLETE
} btstack_gap_event_type_t;

typedef enum {
    BTSTACK_GAP_LE_ROLE_NONE = 0,
    BTSTACK_GAP_LE_ROLE_CENTRAL,
    BTSTACK_GAP_LE_ROLE_PERIPHERAL
} btstack_gap_le_role_t;

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
} btstack_gap_le_adv_config_t;

typedef struct {
    uint16_t interval;
    uint16_t window;
    uint8_t active;
    uint8_t duplicate_filter;
} btstack_gap_le_scan_config_t;

typedef struct {
    btstack_gap_le_role_t role;
    uint16_t connection_handle;
    uint16_t connection_interval;
    uint16_t connection_latency;
    uint16_t supervision_timeout;
    uint8_t address_type;
    uint8_t advertising_event_type;
    int8_t rssi;
    uint32_t passkey;
    uint16_t bond_index;
    uint8_t reason;
    uint8_t data_len;
    uint8_t data[BTSTACK_ADV_DATA_MAX];
} btstack_gap_le_event_data_t;

typedef struct {
    btstack_gap_event_type_t type;
    int status;
    btstack_bd_addr_t address;
    uint32_t class_of_device;
    uint32_t numeric_value;
    int8_t rssi;
    uint8_t stack_working;
    uint8_t name_length;
    char name[BTSTACK_LOCAL_NAME_MAX];
    btstack_gap_le_event_data_t le;
} btstack_gap_event_t;

typedef struct {
    btstack_bd_addr_t local_address;
    btstack_bd_addr_t peer_address;
    char local_name[BTSTACK_LOCAL_NAME_MAX];
    char pin[BTSTACK_GAP_PIN_MAX + 1u];
    uint32_t class_of_device;
    uint8_t ssp_mode;
    bool stack_working;
    bool connectable;
    bool discoverable;
    bool scanning;
    bool acl_connected;
    bool le_advertising;
    bool le_scanning;
    bool le_connecting;
    bool le_connected;
    btstack_gap_le_role_t le_role;
    uint16_t le_connection_handle;
    uint8_t le_peer_address_type;
    btstack_bd_addr_t le_peer_address;
} btstack_gap_state_t;

typedef void (*btstack_gap_event_callback_t)(
    const btstack_gap_event_t *event);

int btstack_gap_init(void);
void btstack_gap_reset(void);
int btstack_gap_set_event_callback(
    btstack_gap_event_callback_t callback);
int btstack_gap_set_power(bool enabled);
bool btstack_gap_is_working(void);
int btstack_gap_get_local_address(btstack_bd_addr_t address);
int btstack_gap_get_state(btstack_gap_state_t *state);
int btstack_gap_set_local_name(const char *name);
int btstack_gap_set_class_of_device(uint32_t class_of_device);
int btstack_gap_set_discoverable(bool enabled);
int btstack_gap_set_connectable(bool enabled);
int btstack_gap_disconnect(void);
int btstack_gap_classic_scan_start(uint8_t duration);
int btstack_gap_classic_scan_stop(void);
bool btstack_gap_classic_is_scanning(void);
int btstack_gap_set_pin(const char *pin);
int btstack_gap_set_ssp_mode(uint8_t mode);
int btstack_gap_pairing_confirm(const btstack_bd_addr_t address, bool accept);
int btstack_gap_le_init(void);
void btstack_gap_le_reset(void);
int btstack_gap_le_adv_configure(const btstack_gap_le_adv_config_t *config);
int btstack_gap_le_adv_start(void);
int btstack_gap_le_adv_stop(void);
int btstack_gap_le_scan_start(const btstack_gap_le_scan_config_t *config);
int btstack_gap_le_scan_stop(void);
int btstack_gap_le_connect(uint8_t address_type, const btstack_bd_addr_t address);
int btstack_gap_le_connect_cancel(void);
int btstack_gap_le_disconnect(void);
int btstack_gap_le_security_init(void);
int btstack_gap_le_pair(void);
int btstack_gap_le_pairing_confirm(uint16_t connection_handle, bool accept);
int btstack_gap_le_passkey_reply(uint16_t connection_handle, uint32_t passkey);
uint8_t btstack_gap_le_bond_count(void);
int btstack_gap_le_bond_delete(uint8_t index);
int btstack_gap_le_bond_delete_all(void);

#ifdef __cplusplus
}
#endif

#endif
