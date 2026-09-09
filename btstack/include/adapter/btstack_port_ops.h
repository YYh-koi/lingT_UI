#ifndef BTSTACK_PORT_OPS_H
#define BTSTACK_PORT_OPS_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTSTACK_PORT_STORAGE_LOCAL_ADDRESS = 0,
    BTSTACK_PORT_STORAGE_CLASSIC_ADDRESS,
    BTSTACK_PORT_STORAGE_BLE_ADDRESS,
    BTSTACK_PORT_STORAGE_LOCAL_NAME,
    BTSTACK_PORT_STORAGE_CLASSIC_NAME,
    BTSTACK_PORT_STORAGE_BLE_NAME,
    BTSTACK_PORT_STORAGE_CLASSIC_BOND,
    BTSTACK_PORT_STORAGE_BLE_BOND,
    BTSTACK_PORT_STORAGE_PROFILE_DATA
} btstack_port_ops_storage_key_t;

typedef enum {
    BTSTACK_PORT_LOG_ERROR = 0,
    BTSTACK_PORT_LOG_WARN,
    BTSTACK_PORT_LOG_INFO,
    BTSTACK_PORT_LOG_DEBUG
} btstack_port_ops_log_level_t;

typedef void (*btstack_port_ops_critical_callback_t)(void *context);
typedef void (*btstack_port_ops_signal_callback_t)(void *context);
typedef int (*btstack_port_ops_schedule_callback_t)(void *context);
typedef uint32_t (*btstack_port_ops_time_ms_callback_t)(void *context);
typedef void (*btstack_port_ops_delay_ms_callback_t)(
    void *context,
    uint32_t delay_ms);
typedef void (*btstack_port_ops_log_callback_t)(
    void *context,
    btstack_port_ops_log_level_t level,
    const char *tag,
    const char *message);

typedef int (*btstack_port_ops_storage_read_callback_t)(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag,
    uint8_t *data,
    uint16_t *data_len);
typedef int (*btstack_port_ops_storage_write_callback_t)(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag,
    const uint8_t *data,
    uint16_t data_len);
typedef int (*btstack_port_ops_storage_delete_callback_t)(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag);

typedef int (*btstack_port_ops_apply_address_callback_t)(
    void *context,
    btstack_transport_t transport,
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t address_type);
typedef int (*btstack_port_ops_random_address_callback_t)(
    void *context,
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type);

typedef struct {
    void *context;
    btstack_port_ops_critical_callback_t enter_critical;
    btstack_port_ops_critical_callback_t exit_critical;
    btstack_port_ops_signal_callback_t signal_event;
    btstack_port_ops_signal_callback_t wake_event_worker;
    btstack_port_ops_schedule_callback_t schedule_event_worker;
    btstack_port_ops_time_ms_callback_t time_ms;
    btstack_port_ops_delay_ms_callback_t delay_ms;
    btstack_port_ops_log_callback_t log;
} btstack_port_ops_system_ops_t;

typedef struct {
    void *context;
    btstack_port_ops_storage_read_callback_t read;
    btstack_port_ops_storage_write_callback_t write;
    btstack_port_ops_storage_delete_callback_t delete_value;
} btstack_port_ops_storage_ops_t;

typedef struct {
    void *context;
    btstack_port_ops_apply_address_callback_t apply_controller_address;
    btstack_port_ops_random_address_callback_t get_random_address;
} btstack_port_ops_controller_ops_t;

typedef struct {
    btstack_port_ops_system_ops_t system;
    btstack_port_ops_storage_ops_t storage;
    btstack_port_ops_controller_ops_t controller;
} btstack_port_ops_t;

/*
 * The adapter layer copies the ops table; callback contexts must remain valid.
 * Set ops during init/quiescent state. enter_critical/exit_critical must be
 * supplied as a pair or both left NULL.
 */
int btstack_port_ops_set_ops(const btstack_port_ops_t *ops);
const btstack_port_ops_t *btstack_port_ops_get_ops(void);

#ifdef __cplusplus
}
#endif

#endif
