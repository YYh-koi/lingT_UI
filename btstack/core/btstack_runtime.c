#include "btstack_runtime.h"

#include <string.h>

#include "adapter/btstack_adapter.h"
#include "adapter/btstack_types.h"
#include "btstack_core_internal.h"
#include "btstack_memory.h"
#include "core/btstack_profile_manager.h"
#include "btstack_run_loop.h"
#include "hci.h"
#include "l2cap.h"
#include "port/btstack_platform.h"

typedef enum {
    BTSTACK_RUNTIME_IDLE = 0,
    BTSTACK_RUNTIME_INITIALIZING,
    BTSTACK_RUNTIME_INITIALIZED,
    BTSTACK_RUNTIME_STARTING,
    BTSTACK_RUNTIME_STARTED,
    BTSTACK_RUNTIME_DEINITIALIZING,
    BTSTACK_RUNTIME_FAILED
} btstack_runtime_state_t;

static btstack_runtime_state_t btstack_runtime_state;

static void btstack_runtime_mark_failed(void)
{
    btstack_platform_enter_critical();
    btstack_runtime_state = BTSTACK_RUNTIME_FAILED;
    btstack_platform_exit_critical();
}

static void btstack_runtime_fail_after_early_init(
    const btstack_platform_t *platform)
{
    btstack_platform_enter_critical();
    btstack_runtime_state = BTSTACK_RUNTIME_DEINITIALIZING;
    btstack_platform_exit_critical();
    platform->stack.late_deinit(platform->stack.context);
    btstack_runtime_mark_failed();
}

static void btstack_runtime_stack_entry(void *argument)
{
    (void)argument;
    btstack_run_loop_execute();
}

int btstack_runtime_init(void)
{
    const btstack_platform_t *platform;
    const btstack_profile_manager_config_t *profile_config = NULL;
    void *context;
    int early_init_completed = 0;
    int status;

    platform = btstack_platform_get();
    if (platform == NULL) return BTSTACK_STATUS_NOT_READY;

    btstack_platform_enter_critical();
    if ((btstack_runtime_state == BTSTACK_RUNTIME_INITIALIZED) ||
        (btstack_runtime_state == BTSTACK_RUNTIME_STARTED)) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_OK;
    }
    if (btstack_runtime_state == BTSTACK_RUNTIME_INITIALIZING) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_BUSY;
    }
    if (btstack_runtime_state != BTSTACK_RUNTIME_IDLE) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_runtime_state = BTSTACK_RUNTIME_INITIALIZING;
    btstack_platform_exit_critical();

    context = platform->stack.context;
    status = platform->stack.early_init(context);
    if (status != BTSTACK_STATUS_OK) goto failed;
    early_init_completed = 1;

    btstack_run_loop_init(platform->stack.get_run_loop(context));
    btstack_memory_init();
    hci_init(platform->stack.get_hci_transport(context),
        platform->stack.get_hci_transport_config(context));
    hci_set_control(platform->controller.control);
    if (platform->stack.after_hci_init != NULL) {
        status = platform->stack.after_hci_init(context);
        if (status != BTSTACK_STATUS_OK) goto failed;
    }
    l2cap_init();
    status = btstack_adapter_setup();
    if (status != BTSTACK_STATUS_OK) goto failed;
    status = btstack_core_event_mapper_init();
    if (status != BTSTACK_STATUS_OK) goto failed;
    if (platform->stack.get_profile_manager_config != NULL) {
        profile_config =
            platform->stack.get_profile_manager_config(context);
    }
    status = btstack_profile_manager_init(profile_config);
    if (status != BTSTACK_STATUS_OK) goto failed;
    status = btstack_core_dispatcher_init();
    if (status != BTSTACK_STATUS_OK) goto failed;

    btstack_platform_enter_critical();
    btstack_runtime_state = BTSTACK_RUNTIME_INITIALIZED;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;

failed:
    if (early_init_completed) {
        btstack_runtime_fail_after_early_init(platform);
    } else {
        btstack_runtime_mark_failed();
    }
    return (status == BTSTACK_STATUS_OK) ? BTSTACK_STATUS_ERROR : status;
}

int btstack_runtime_start(void)
{
    btstack_worker_config_t config = {0u, 0u};
    int status;

    btstack_platform_enter_critical();
    if (btstack_runtime_state == BTSTACK_RUNTIME_STARTED) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_OK;
    }
    if (btstack_runtime_state == BTSTACK_RUNTIME_STARTING) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_BUSY;
    }
    if (btstack_runtime_state != BTSTACK_RUNTIME_INITIALIZED) {
        btstack_platform_exit_critical();
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_runtime_state = BTSTACK_RUNTIME_STARTING;
    btstack_platform_exit_critical();

    status = btstack_platform_worker_validate(BTSTACK_WORKER_STACK, &config);
    if (status == BTSTACK_STATUS_OK) {
        status = btstack_platform_worker_start(BTSTACK_WORKER_STACK,
            btstack_runtime_stack_entry, NULL, &config);
    }
    if (status != BTSTACK_STATUS_OK) {
        const btstack_platform_t *platform = btstack_platform_get();
        if (platform != NULL) {
            btstack_runtime_fail_after_early_init(platform);
        } else {
            btstack_runtime_mark_failed();
        }
        return status;
    }

    btstack_platform_enter_critical();
    btstack_runtime_state = BTSTACK_RUNTIME_STARTED;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_runtime_started(void)
{
    int started;
    btstack_platform_enter_critical();
    started = btstack_runtime_state == BTSTACK_RUNTIME_STARTED;
    btstack_platform_exit_critical();
    return started;
}

void btstack_runtime_get_diagnostics(
    btstack_runtime_diagnostics_t *diagnostics)
{
    const btstack_platform_t *platform;
    btstack_stack_queue_diagnostics_t stack_diagnostics;

    if (diagnostics == NULL) return;
    memset(diagnostics, 0, sizeof(*diagnostics));
    btstack_platform_enter_critical();
    if ((btstack_runtime_state != BTSTACK_RUNTIME_INITIALIZED) &&
        (btstack_runtime_state != BTSTACK_RUNTIME_STARTING) &&
        (btstack_runtime_state != BTSTACK_RUNTIME_STARTED)) {
        btstack_platform_exit_critical();
        return;
    }
    platform = btstack_platform_get();
    if ((platform == NULL) ||
        (platform->stack.get_diagnostics == NULL)) {
        btstack_platform_exit_critical();
        return;
    }
    memset(&stack_diagnostics, 0, sizeof(stack_diagnostics));
    platform->stack.get_diagnostics(platform->stack.context,
        &stack_diagnostics);
    diagnostics->current_depth = stack_diagnostics.current_depth;
    diagnostics->max_depth = stack_diagnostics.max_depth;
    diagnostics->full_count = stack_diagnostics.full_count;
    btstack_platform_exit_critical();
}
