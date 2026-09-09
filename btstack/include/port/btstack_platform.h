#ifndef BTSTACK_PLATFORM_H
#define BTSTACK_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_port_ops.h"
#include "btstack_control.h"
#include "port/btstack_stack_ops.h"
#include "port/btstack_worker_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_PLATFORM_ABI_VERSION 1u

typedef enum {
    BTSTACK_PLATFORM_OK = 0,
    BTSTACK_PLATFORM_ERROR_NULL = -1,
    BTSTACK_PLATFORM_ERROR_ABI = -2,
    BTSTACK_PLATFORM_ERROR_SIZE = -3,
    BTSTACK_PLATFORM_ERROR_REQUIRED_OP = -4,
    BTSTACK_PLATFORM_ERROR_ALREADY_ACTIVE = -5
} btstack_platform_status_t;

typedef struct {
    btstack_port_ops_controller_ops_t identity;
    const btstack_control_t *control;
} btstack_platform_controller_ops_t;

/*
 * Descriptor storage, its name, nested ops tables, and every callback context
 * referenced by those ops tables must be static or application-owned and must
 * remain valid after successful activation for the entire process or firmware
 * lifetime. late_deinit releases only running resources created by early_init;
 * it must not release the descriptor, name, nested ops, or callback contexts.
 */
typedef struct {
    uint16_t abi_version;
    uint16_t struct_size;
    const char *name;
    btstack_port_ops_system_ops_t system;
    btstack_worker_ops_t workers;
    btstack_port_ops_storage_ops_t storage;
    btstack_platform_controller_ops_t controller;
    btstack_stack_ops_t stack;
} btstack_platform_t;

typedef struct {
    btstack_platform_status_t validation_status;
} btstack_platform_diagnostics_t;

const btstack_platform_t *btstack_platform_default(void);

btstack_platform_status_t btstack_platform_activate(
    const btstack_platform_t *platform);
const btstack_platform_t *btstack_platform_get(void);
void btstack_platform_get_diagnostics(
    btstack_platform_diagnostics_t *diagnostics);

void btstack_platform_enter_critical(void);
void btstack_platform_exit_critical(void);
uint32_t btstack_platform_time_ms(void);
int btstack_platform_worker_validate(
    btstack_worker_id_t id,
    const btstack_worker_config_t *config);
int btstack_platform_worker_start(
    btstack_worker_id_t id,
    btstack_worker_entry_t entry,
    void *argument,
    const btstack_worker_config_t *config);
int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals);
uint32_t btstack_platform_wait(
    btstack_worker_id_t id,
    uint32_t mask,
    uint32_t timeout_ms);
int btstack_platform_worker_stop(btstack_worker_id_t id);
bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback);

#ifdef __cplusplus
}
#endif

#endif
