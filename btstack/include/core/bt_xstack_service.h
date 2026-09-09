#ifndef BT_XSTACK_SERVICE_H
#define BT_XSTACK_SERVICE_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_XSTACK_SERVICE_EVENT_DEVICE_STATE = 0,
    BT_XSTACK_SERVICE_EVENT_POWER_STATE,
    BT_XSTACK_SERVICE_EVENT_SCAN_RESULT,
    BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE,
    BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST,
    BT_XSTACK_SERVICE_EVENT_PAIRED
} bt_xstack_service_event_id_t;

typedef struct {
    bt_xstack_service_event_id_t id;
    bt_xstack_addr_t address;
    /*
     * DEVICE_STATE: values[0] mask, power=bit0, discoverable=bit1,
     * scanning=bit3.
     * POWER_STATE: values[0] is 0 (off) or 1 (on).
     * SCAN_RESULT: values[0] is the one-based result index; address,
     * class_of_device, rssi and name describe the discovered device.
     * SCAN_COMPLETE: values[0] is reserved and currently zero.
     * PAIRING_REQUEST: values[0] is the numeric confirmation value.
     * PAIRED: values[0] is the native status (0 means success).
     */
    uint32_t values[3];
    uint8_t value_count;
    uint32_t class_of_device;
    int8_t rssi;
    char name[BT_XSTACK_NAME_MAX];
} bt_xstack_service_event_t;

typedef struct {
    bt_xstack_profile_t profile;
    bt_xstack_connection_state_t state;
    bt_xstack_addr_t address;
    bool unexpected_loss;
} bt_xstack_connection_event_t;

typedef void (*bt_xstack_service_callback_t)(
    const bt_xstack_service_event_t *event,
    void *context);
typedef void (*bt_xstack_connection_callback_t)(
    const bt_xstack_connection_event_t *event,
    void *context);

bool bt_xstack_is_ready(void);
bt_xstack_result_t bt_xstack_enable(void);
bt_xstack_result_t bt_xstack_disable(void);
bt_xstack_result_t bt_xstack_get_state(bt_xstack_state_t *state);
bt_xstack_result_t bt_xstack_get_diagnostics(
    bt_xstack_diagnostics_t *diagnostics);
/* This compile-time version query is intentionally valid before init. */
bt_xstack_result_t bt_xstack_get_version(char *version, uint16_t capacity);
bt_xstack_result_t bt_xstack_get_profile_mask(uint16_t *profile_mask);
bt_xstack_result_t bt_xstack_set_profile_mask(uint16_t profile_mask);
bt_xstack_result_t bt_xstack_get_local_address(bt_xstack_addr_t *address);
bt_xstack_result_t bt_xstack_get_local_name(char *name, uint16_t capacity);
bt_xstack_result_t bt_xstack_set_local_name(
    const char *name,
    bool append_address_suffix);
bt_xstack_result_t bt_xstack_get_pin(char *pin, uint16_t capacity);
bt_xstack_result_t bt_xstack_set_pin(const char *pin);
bt_xstack_result_t bt_xstack_get_ssp_mode(uint8_t *mode);
bt_xstack_result_t bt_xstack_set_ssp_mode(uint8_t mode);
bt_xstack_result_t bt_xstack_pairing_confirm(
    const bt_xstack_addr_t *address,
    bool accept);
bt_xstack_result_t bt_xstack_set_discoverable(bool enabled);
bt_xstack_result_t bt_xstack_set_connectable(bool enabled);
bt_xstack_result_t bt_xstack_get_class_of_device(
    uint32_t *class_of_device);
bt_xstack_result_t bt_xstack_set_class_of_device(
    uint32_t class_of_device);
bt_xstack_result_t bt_xstack_get_paired_device_count(uint8_t *count);
/* Paired-device indexes preserve the AT protocol's one-based range 1..8. */
bt_xstack_result_t bt_xstack_get_paired_device(
    uint8_t index,
    bt_xstack_paired_device_t *device);
bt_xstack_result_t bt_xstack_clear_paired_device_by_index(uint8_t index);
bt_xstack_result_t bt_xstack_clear_paired_device_by_address(
    const bt_xstack_addr_t *address);
bt_xstack_result_t bt_xstack_clear_paired_devices(void);
bt_xstack_result_t bt_xstack_get_tdl_mode(uint8_t *mode);
bt_xstack_result_t bt_xstack_set_tdl_mode(uint8_t mode);
bt_xstack_result_t bt_xstack_scan_start(
    uint8_t period_seconds,
    const char *name_filter);
bt_xstack_result_t bt_xstack_scan_stop(void);
bt_xstack_result_t bt_xstack_disconnect_all(void);
bt_xstack_result_t bt_xstack_connect(
    const bt_xstack_addr_t *address,
    bt_xstack_profile_t profile);
bt_xstack_result_t bt_xstack_connect_last(bt_xstack_profile_t profile);
bt_xstack_result_t bt_xstack_disconnect(bt_xstack_profile_t profile);
bt_xstack_result_t bt_xstack_get_connection_state(
    bt_xstack_profile_t profile,
    bt_xstack_connection_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
