#include "port/btstack_platform.h"

static const btstack_platform_t *btstack_active_platform;
static btstack_platform_diagnostics_t btstack_platform_diagnostics = {
    BTSTACK_PLATFORM_ERROR_NULL
};

static btstack_platform_status_t btstack_platform_record_status(
    btstack_platform_status_t status)
{
    btstack_platform_diagnostics.validation_status = status;
    return status;
}

static btstack_platform_status_t btstack_platform_validate(
    const btstack_platform_t *platform)
{
    if (platform == NULL) {
        return BTSTACK_PLATFORM_ERROR_NULL;
    }
    if (platform->abi_version != BTSTACK_PLATFORM_ABI_VERSION) {
        return BTSTACK_PLATFORM_ERROR_ABI;
    }
    if (platform->struct_size < sizeof(*platform)) {
        return BTSTACK_PLATFORM_ERROR_SIZE;
    }
    if ((platform->name == NULL) || (platform->name[0] == '\0')) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    if ((platform->system.enter_critical == NULL) ||
        (platform->system.exit_critical == NULL) ||
        (platform->system.time_ms == NULL)) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    if ((platform->workers.validate_config == NULL) ||
        (platform->workers.start == NULL) ||
        (platform->workers.notify == NULL) ||
        (platform->workers.wait == NULL) ||
        (platform->workers.stop == NULL)) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    if ((platform->controller.control == NULL) ||
        (platform->controller.control->init == NULL)) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    if ((platform->stack.early_init == NULL) ||
        (platform->stack.late_deinit == NULL) ||
        (platform->stack.get_run_loop == NULL) ||
        (platform->stack.get_hci_transport == NULL) ||
        (platform->stack.get_hci_transport_config == NULL) ||
        (platform->stack.post_callback == NULL)) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    if ((platform->stack.get_run_loop(platform->stack.context) == NULL) ||
        (platform->stack.get_hci_transport(platform->stack.context) == NULL) ||
        (platform->stack.get_hci_transport_config(
            platform->stack.context) == NULL)) {
        return BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    }
    return BTSTACK_PLATFORM_OK;
}

btstack_platform_status_t btstack_platform_activate(
    const btstack_platform_t *platform)
{
    btstack_platform_status_t status;
    btstack_port_ops_t compatibility_ops;

    compatibility_ops.system = (btstack_port_ops_system_ops_t){0};
    compatibility_ops.storage = (btstack_port_ops_storage_ops_t){0};
    compatibility_ops.controller = (btstack_port_ops_controller_ops_t){0};

    if (btstack_active_platform != NULL) {
        return btstack_platform_record_status(
            BTSTACK_PLATFORM_ERROR_ALREADY_ACTIVE);
    }

    status = btstack_platform_validate(platform);
    if (status != BTSTACK_PLATFORM_OK) {
        return btstack_platform_record_status(status);
    }

    compatibility_ops.system = platform->system;
    compatibility_ops.storage = platform->storage;
    compatibility_ops.controller = platform->controller.identity;
    if (btstack_port_ops_set_ops(&compatibility_ops) != BTSTACK_STATUS_OK) {
        return btstack_platform_record_status(
            BTSTACK_PLATFORM_ERROR_REQUIRED_OP);
    }

    btstack_active_platform = platform;
    return btstack_platform_record_status(BTSTACK_PLATFORM_OK);
}

const btstack_platform_t *btstack_platform_get(void)
{
    return btstack_active_platform;
}

void btstack_platform_get_diagnostics(
    btstack_platform_diagnostics_t *diagnostics)
{
    if (diagnostics != NULL) {
        *diagnostics = btstack_platform_diagnostics;
    }
}

void btstack_platform_enter_critical(void)
{
    const btstack_port_ops_t *legacy_ops;

    if (btstack_active_platform != NULL) {
        btstack_active_platform->system.enter_critical(
            btstack_active_platform->system.context);
        return;
    }

    legacy_ops = btstack_port_ops_get_ops();
    if ((legacy_ops != NULL) &&
        (legacy_ops->system.enter_critical != NULL) &&
        (legacy_ops->system.exit_critical != NULL)) {
        legacy_ops->system.enter_critical(legacy_ops->system.context);
    }
}

void btstack_platform_exit_critical(void)
{
    const btstack_port_ops_t *legacy_ops;

    if (btstack_active_platform != NULL) {
        btstack_active_platform->system.exit_critical(
            btstack_active_platform->system.context);
        return;
    }

    legacy_ops = btstack_port_ops_get_ops();
    if ((legacy_ops != NULL) &&
        (legacy_ops->system.enter_critical != NULL) &&
        (legacy_ops->system.exit_critical != NULL)) {
        legacy_ops->system.exit_critical(legacy_ops->system.context);
    }
}

uint32_t btstack_platform_time_ms(void)
{
    if (btstack_active_platform == NULL) {
        return 0u;
    }
    return btstack_active_platform->system.time_ms(
        btstack_active_platform->system.context);
}

static int btstack_platform_worker_id_valid(btstack_worker_id_t id)
{
    return ((unsigned int)id < (unsigned int)BTSTACK_WORKER_COUNT);
}

int btstack_platform_worker_validate(
    btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    if ((btstack_active_platform == NULL) || (config == NULL) ||
        !btstack_platform_worker_id_valid(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_active_platform->workers.validate_config(
        btstack_active_platform->workers.context, id, config);
}

int btstack_platform_worker_start(
    btstack_worker_id_t id,
    btstack_worker_entry_t entry,
    void *argument,
    const btstack_worker_config_t *config)
{
    if ((btstack_active_platform == NULL) || (entry == NULL) ||
        (config == NULL) || !btstack_platform_worker_id_valid(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_active_platform->workers.start(
        btstack_active_platform->workers.context,
        id, entry, argument, config);
}

int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    if ((btstack_active_platform == NULL) ||
        !btstack_platform_worker_id_valid(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_active_platform->workers.notify(
        btstack_active_platform->workers.context,
        id,
        signals);
}

uint32_t btstack_platform_wait(
    btstack_worker_id_t id,
    uint32_t mask,
    uint32_t timeout_ms)
{
    if ((btstack_active_platform == NULL) ||
        !btstack_platform_worker_id_valid(id)) {
        return 0u;
    }
    return btstack_active_platform->workers.wait(
        btstack_active_platform->workers.context,
        id,
        mask,
        timeout_ms);
}

int btstack_platform_worker_stop(btstack_worker_id_t id)
{
    if ((btstack_active_platform == NULL) ||
        !btstack_platform_worker_id_valid(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_active_platform->workers.stop(
        btstack_active_platform->workers.context, id);
}

bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback)
{
    if ((btstack_active_platform == NULL) || (callback == NULL)) {
        return false;
    }
    return btstack_active_platform->stack.post_callback(
        btstack_active_platform->stack.context,
        callback);
}
