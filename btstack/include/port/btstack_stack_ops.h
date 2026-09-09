#ifndef BTSTACK_STACK_OPS_H
#define BTSTACK_STACK_OPS_H

#include <stdbool.h>
#include <stdint.h>

#include "btstack_run_loop.h"
#include "core/btstack_profile_manager.h"
#include "hci_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t current_depth;
    uint32_t max_depth;
    uint32_t full_count;
} btstack_stack_queue_diagnostics_t;

typedef struct {
    void *context;
    int (*early_init)(void *context);
    void (*late_deinit)(void *context);
    const btstack_run_loop_t *(*get_run_loop)(void *context);
    const hci_transport_t *(*get_hci_transport)(void *context);
    const void *(*get_hci_transport_config)(void *context);
    const btstack_profile_manager_config_t *
        (*get_profile_manager_config)(void *context);
    int (*after_hci_init)(void *context);
    bool (*post_callback)(
        void *context,
        btstack_context_callback_registration_t *callback);
    /* Must be bounded and non-blocking; common code may call it while the
     * platform system critical section is held. */
    void (*get_diagnostics)(
        void *context,
        btstack_stack_queue_diagnostics_t *diagnostics);
} btstack_stack_ops_t;

#ifdef __cplusplus
}
#endif

#endif
