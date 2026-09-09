#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adapter/btstack_port_ops.h"
#include "adapter/btstack_property.h"
#include "events/btstack_event.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"
#include "../btstack_core_internal.h"

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                          \
            return 1;                                                         \
        }                                                                     \
    } while (0)

typedef struct {
    unsigned enter_count;
    unsigned exit_count;
    unsigned critical_depth;
    unsigned critical_violation_count;
    unsigned app_event_count;
    btstack_worker_id_t notify_id;
    uint32_t notify_signals;
    btstack_worker_id_t wait_id;
    uint32_t wait_mask;
    uint32_t wait_timeout_ms;
    uint32_t time_ms;
    unsigned time_call_count;
    btstack_context_callback_registration_t *posted_callback;
} test_context_t;

static test_context_t test_context;
static const btstack_run_loop_t test_run_loop = {0};
static const hci_transport_t test_transport = {0};
static const hci_transport_config_t test_transport_config = {
    HCI_TRANSPORT_CONFIG_UART
};

static void test_control_init(const void *config)
{
    (void) config;
}

static const btstack_control_t test_control = {
    .init = test_control_init
};
static const btstack_control_t test_control_without_init = {0};

static void test_enter_critical(void *context)
{
    test_context_t *test = (test_context_t *) context;

    test->enter_count++;
    test->critical_depth++;
    if (test->critical_depth != 1u) {
        test->critical_violation_count++;
    }
}

static void test_exit_critical(void *context)
{
    test_context_t *test = (test_context_t *) context;

    test->exit_count++;
    if (test->critical_depth != 1u) {
        test->critical_violation_count++;
    }
    if (test->critical_depth != 0u) {
        test->critical_depth--;
    }
}

static uint32_t test_time_ms(void *context)
{
    test_context_t *test = (test_context_t *) context;

    test->time_call_count++;
    return test->time_ms;
}

static void test_delay_ms(void *context, uint32_t delay_ms)
{
    (void) context;
    (void) delay_ms;
}

static int test_storage_read(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag,
    uint8_t *data,
    uint16_t *data_len)
{
    (void) context;
    (void) key;
    (void) tag;
    (void) data;
    (void) data_len;
    return BTSTACK_STATUS_OK;
}

static int test_storage_write(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag,
    const uint8_t *data,
    uint16_t data_len)
{
    (void) context;
    (void) key;
    (void) tag;
    (void) data;
    (void) data_len;
    return BTSTACK_STATUS_OK;
}

static int test_storage_delete(
    void *context,
    btstack_port_ops_storage_key_t key,
    uint8_t tag)
{
    (void) context;
    (void) key;
    (void) tag;
    return BTSTACK_STATUS_OK;
}

static int test_apply_address(
    void *context,
    btstack_transport_t transport,
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t address_type)
{
    (void) context;
    (void) transport;
    (void) address;
    (void) address_type;
    return BTSTACK_STATUS_OK;
}

static int test_random_address(
    void *context,
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type)
{
    (void) context;
    (void) address;
    (void) address_type;
    return BTSTACK_STATUS_OK;
}

static int test_worker_validate(
    void *context,
    btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    (void) context;
    (void) id;
    (void) config;
    return BTSTACK_STATUS_OK;
}

static int test_worker_start(
    void *context,
    btstack_worker_id_t id,
    btstack_worker_entry_t entry,
    void *argument,
    const btstack_worker_config_t *config)
{
    (void) context;
    (void) id;
    (void) entry;
    (void) argument;
    (void) config;
    return BTSTACK_STATUS_OK;
}

static int test_worker_notify(
    void *context,
    btstack_worker_id_t id,
    uint32_t signals)
{
    test_context_t *test = (test_context_t *) context;
    if (test->critical_depth != 0u) {
        test->critical_violation_count++;
    }
    test->notify_id = id;
    test->notify_signals = signals;
    return 37;
}

static void test_app_event(
    btstack_transport_t transport,
    btstack_event_t event,
    const uint8_t *data,
    uint16_t data_len)
{
    (void) transport;
    (void) event;
    (void) data;
    (void) data_len;
    if (test_context.critical_depth != 0u) {
        test_context.critical_violation_count++;
    }
    test_context.app_event_count++;
}

static uint32_t test_worker_wait(
    void *context,
    btstack_worker_id_t id,
    uint32_t signal_mask,
    uint32_t timeout_ms)
{
    test_context_t *test = (test_context_t *) context;
    test->wait_id = id;
    test->wait_mask = signal_mask;
    test->wait_timeout_ms = timeout_ms;
    return signal_mask;
}

static int test_worker_stop(void *context, btstack_worker_id_t id)
{
    (void) context;
    (void) id;
    return BTSTACK_STATUS_OK;
}

static int test_early_init(void *context)
{
    (void) context;
    return BTSTACK_STATUS_OK;
}

static void test_late_deinit(void *context)
{
    (void) context;
}

static const btstack_run_loop_t *test_get_run_loop(void *context)
{
    (void) context;
    return &test_run_loop;
}

static const btstack_run_loop_t *test_get_null_run_loop(void *context)
{
    (void) context;
    return NULL;
}

static const hci_transport_t *test_get_transport(void *context)
{
    (void) context;
    return &test_transport;
}

static const hci_transport_t *test_get_null_transport(void *context)
{
    (void) context;
    return NULL;
}

static const void *test_get_transport_config(void *context)
{
    (void) context;
    return &test_transport_config;
}

static const void *test_get_null_transport_config(void *context)
{
    (void) context;
    return NULL;
}

static const btstack_profile_manager_config_t *test_get_profiles(void *context)
{
    (void) context;
    return NULL;
}

static int test_after_hci_init(void *context)
{
    (void) context;
    return BTSTACK_STATUS_OK;
}

static bool test_post_callback(
    void *context,
    btstack_context_callback_registration_t *callback)
{
    ((test_context_t *) context)->posted_callback = callback;
    return true;
}

static btstack_platform_t make_valid_platform(const char *name)
{
    btstack_platform_t platform = {0};

    platform.abi_version = BTSTACK_PLATFORM_ABI_VERSION;
    platform.struct_size = (uint16_t) sizeof(platform);
    platform.name = name;
    platform.system.context = &test_context;
    platform.system.enter_critical = test_enter_critical;
    platform.system.exit_critical = test_exit_critical;
    platform.system.time_ms = test_time_ms;
    platform.system.delay_ms = test_delay_ms;
    platform.workers.context = &test_context;
    platform.workers.validate_config = test_worker_validate;
    platform.workers.start = test_worker_start;
    platform.workers.notify = test_worker_notify;
    platform.workers.wait = test_worker_wait;
    platform.workers.stop = test_worker_stop;
    platform.storage.context = &test_context;
    platform.storage.read = test_storage_read;
    platform.storage.write = test_storage_write;
    platform.storage.delete_value = test_storage_delete;
    platform.controller.identity.context = &test_context;
    platform.controller.identity.apply_controller_address = test_apply_address;
    platform.controller.identity.get_random_address = test_random_address;
    platform.controller.control = &test_control;
    platform.stack.context = &test_context;
    platform.stack.early_init = test_early_init;
    platform.stack.late_deinit = test_late_deinit;
    platform.stack.get_run_loop = test_get_run_loop;
    platform.stack.get_hci_transport = test_get_transport;
    platform.stack.get_hci_transport_config = test_get_transport_config;
    platform.stack.get_profile_manager_config = test_get_profiles;
    platform.stack.after_hci_init = test_after_hci_init;
    platform.stack.post_callback = test_post_callback;
    return platform;
}

static int seed_legacy_sentinel(btstack_port_ops_t *snapshot)
{
    btstack_port_ops_t sentinel = {0};

    sentinel.system.context = &test_context;
    sentinel.system.enter_critical = test_enter_critical;
    sentinel.system.exit_critical = test_exit_critical;
    sentinel.system.time_ms = test_time_ms;
    sentinel.storage.context = &test_context;
    sentinel.storage.read = test_storage_read;
    sentinel.controller.context = &test_context;
    sentinel.controller.apply_controller_address = test_apply_address;

    CHECK(btstack_port_ops_set_ops(&sentinel) == BTSTACK_STATUS_OK);
    *snapshot = *btstack_port_ops_get_ops();
    CHECK(snapshot->system.context != NULL);
    CHECK(snapshot->system.enter_critical != NULL);
    CHECK(snapshot->storage.read != NULL);
    CHECK(snapshot->controller.apply_controller_address != NULL);
    return 0;
}

static int check_invalid_activation_case(const char *case_name)
{
    btstack_platform_t invalid = make_valid_platform(case_name);
    const btstack_platform_t *candidate = &invalid;
    btstack_platform_status_t expected = BTSTACK_PLATFORM_ERROR_REQUIRED_OP;
    btstack_port_ops_t legacy_before;

    if (strcmp(case_name, "null-platform") == 0) {
        candidate = NULL;
        expected = BTSTACK_PLATFORM_ERROR_NULL;
    }
    else if (strcmp(case_name, "bad-version") == 0) {
        invalid.abi_version++;
        expected = BTSTACK_PLATFORM_ERROR_ABI;
    }
    else if (strcmp(case_name, "bad-size") == 0) {
        invalid.struct_size = (uint16_t) (sizeof(invalid) - 1u);
        expected = BTSTACK_PLATFORM_ERROR_SIZE;
    }
    else if (strcmp(case_name, "bad-name") == 0) {
        invalid.name = "";
    }
    else if (strcmp(case_name, "missing-enter-critical") == 0) {
        invalid.system.enter_critical = NULL;
    }
    else if (strcmp(case_name, "missing-exit-critical") == 0) {
        invalid.system.exit_critical = NULL;
    }
    else if (strcmp(case_name, "missing-time") == 0) {
        invalid.system.time_ms = NULL;
    }
    else if (strcmp(case_name, "missing-worker-validate") == 0) {
        invalid.workers.validate_config = NULL;
    }
    else if (strcmp(case_name, "missing-worker-start") == 0) {
        invalid.workers.start = NULL;
    }
    else if (strcmp(case_name, "missing-worker-notify") == 0) {
        invalid.workers.notify = NULL;
    }
    else if (strcmp(case_name, "missing-worker-wait") == 0) {
        invalid.workers.wait = NULL;
    }
    else if (strcmp(case_name, "missing-worker-stop") == 0) {
        invalid.workers.stop = NULL;
    }
    else if (strcmp(case_name, "missing-control") == 0) {
        invalid.controller.control = NULL;
    }
    else if (strcmp(case_name, "null-control-init") == 0) {
        invalid.controller.control = &test_control_without_init;
    }
    else if (strcmp(case_name, "missing-early-init") == 0) {
        invalid.stack.early_init = NULL;
    }
    else if (strcmp(case_name, "missing-late-deinit") == 0) {
        invalid.stack.late_deinit = NULL;
    }
    else if (strcmp(case_name, "missing-run-loop-getter") == 0) {
        invalid.stack.get_run_loop = NULL;
    }
    else if (strcmp(case_name, "null-run-loop-result") == 0) {
        invalid.stack.get_run_loop = test_get_null_run_loop;
    }
    else if (strcmp(case_name, "missing-transport-getter") == 0) {
        invalid.stack.get_hci_transport = NULL;
    }
    else if (strcmp(case_name, "null-transport-result") == 0) {
        invalid.stack.get_hci_transport = test_get_null_transport;
    }
    else if (strcmp(case_name, "missing-transport-config-getter") == 0) {
        invalid.stack.get_hci_transport_config = NULL;
    }
    else if (strcmp(case_name, "null-transport-config-result") == 0) {
        invalid.stack.get_hci_transport_config = test_get_null_transport_config;
    }
    else if (strcmp(case_name, "missing-stack-post") == 0) {
        invalid.stack.post_callback = NULL;
    }
    else {
        fprintf(stderr, "Unknown test case: %s\n", case_name);
        return 1;
    }

    CHECK(seed_legacy_sentinel(&legacy_before) == 0);
    CHECK(btstack_platform_activate(candidate) == expected);
    CHECK(btstack_platform_get() == NULL);
    CHECK(memcmp(&legacy_before, btstack_port_ops_get_ops(),
                 sizeof(legacy_before)) == 0);
    return 0;
}

static int check_legacy_critical_fallback(void)
{
    btstack_port_ops_t legacy_before;
    bt_xstack_state_t state;
    uint8_t property_address[BTSTACK_BD_ADDR_LEN] = {
        1u, 2u, 3u, 4u, 5u, 6u
    };
    uint8_t property_snapshot[BTSTACK_BD_ADDR_LEN] = {0};

    CHECK(btstack_platform_get() == NULL);
    CHECK(seed_legacy_sentinel(&legacy_before) == 0);

    btstack_platform_enter_critical();
    CHECK(test_context.critical_depth == 1u);
    btstack_platform_exit_critical();

    btstack_core_state_set_value(
        BT_XSTACK_REQUEST_SET_PROFILE_MASK, 0x5678u);
    memset(&state, 0, sizeof(state));
    CHECK(btstack_core_copy_state(&state) == 0);
    CHECK(state.profile_mask == 0x5678u);

    btstack_property_reset();
    CHECK(btstack_property_set_classic_address(
              property_address, false) == BTSTACK_STATUS_OK);
    CHECK(btstack_property_get_classic_address(property_snapshot));
    CHECK(memcmp(property_snapshot, property_address,
                 sizeof(property_snapshot)) == 0);

    btstack_event_reset();
    btstack_event_register_app_callback(test_app_event);
    CHECK(btstack_event_post(
              BTSTACK_TRANSPORT_CLASSIC,
              BTSTACK_EVENT_READY,
              NULL,
              0u) == BTSTACK_STATUS_OK);
    CHECK(test_context.app_event_count == 1u);
    CHECK(test_context.enter_count > 2u);
    CHECK(test_context.critical_depth == 0u);
    CHECK(test_context.enter_count == test_context.exit_count);
    CHECK(test_context.critical_violation_count == 0u);
    return 0;
}

int main(int argc, char **argv)
{
    btstack_platform_t valid = make_valid_platform("host-test");
    btstack_platform_t other_valid = make_valid_platform("other-host-test");
    btstack_platform_t bad_version = valid;
    btstack_platform_diagnostics_t diagnostics;
    btstack_port_ops_t active_legacy;
    const btstack_port_ops_t *legacy;
    btstack_context_callback_registration_t callback = {0};
    bt_xstack_state_t state;
    uint8_t property_address[BTSTACK_BD_ADDR_LEN] = {
        1u, 2u, 3u, 4u, 5u, 6u
    };
    uint8_t property_snapshot[BTSTACK_BD_ADDR_LEN] = {0};
    unsigned enter_before;

    if (argc == 2) {
        if (strcmp(argv[1], "legacy-critical") == 0) {
            return check_legacy_critical_fallback();
        }
        return check_invalid_activation_case(argv[1]);
    }

    CHECK(BTSTACK_WORKER_WAIT_FOREVER == UINT32_MAX);
    CHECK(BTSTACK_WORKER_API == 0);
    CHECK(BTSTACK_WORKER_EVENT == 1);
    CHECK(BTSTACK_WORKER_STACK == 2);
    CHECK(BTSTACK_WORKER_AT == 3);
    CHECK(BTSTACK_WORKER_COUNT == 4);
    CHECK(BTSTACK_WORKER_SIGNAL_WORK == (1u << 0));
    CHECK(BTSTACK_WORKER_SIGNAL_COMMAND_SLOT == (1u << 1));
    CHECK(BTSTACK_WORKER_SIGNAL_REQUEST_SPACE == (1u << 2));
    CHECK(BTSTACK_WORKER_SIGNAL_COMPLETION == (1u << 3));

    btstack_platform_enter_critical();
    btstack_platform_exit_critical();
    CHECK(test_context.enter_count == 0u);
    CHECK(test_context.exit_count == 0u);
    CHECK(btstack_platform_notify(BTSTACK_WORKER_EVENT,
                                  BTSTACK_WORKER_SIGNAL_WORK) ==
          BTSTACK_STATUS_NOT_READY);
    CHECK(btstack_platform_wait(BTSTACK_WORKER_STACK,
                                BTSTACK_WORKER_SIGNAL_WORK,
                                1u) == 0u);
    CHECK(btstack_platform_time_ms() == 0u);
    CHECK(!btstack_platform_post_stack_callback(NULL));
    CHECK(!btstack_platform_post_stack_callback(&callback));

    CHECK(btstack_platform_activate(&valid) == BTSTACK_PLATFORM_OK);
    CHECK(btstack_platform_get() == &valid);
    btstack_platform_get_diagnostics(&diagnostics);
    CHECK(diagnostics.validation_status == BTSTACK_PLATFORM_OK);
    test_context.time_ms = 4567u;
    CHECK(btstack_platform_time_ms() == 4567u);
    CHECK(test_context.time_call_count == 1u);

    legacy = btstack_port_ops_get_ops();
    CHECK(legacy->system.context == valid.system.context);
    CHECK(legacy->system.enter_critical == valid.system.enter_critical);
    CHECK(legacy->system.exit_critical == valid.system.exit_critical);
    CHECK(legacy->system.time_ms == valid.system.time_ms);
    CHECK(legacy->storage.context == valid.storage.context);
    CHECK(legacy->storage.read == valid.storage.read);
    CHECK(legacy->storage.write == valid.storage.write);
    CHECK(legacy->storage.delete_value == valid.storage.delete_value);
    CHECK(legacy->controller.context == valid.controller.identity.context);
    CHECK(legacy->controller.apply_controller_address ==
          valid.controller.identity.apply_controller_address);
    CHECK(legacy->controller.get_random_address ==
          valid.controller.identity.get_random_address);

    btstack_platform_enter_critical();
    btstack_platform_exit_critical();
    CHECK(test_context.enter_count == 1u);
    CHECK(test_context.exit_count == 1u);
    CHECK(btstack_platform_notify(BTSTACK_WORKER_EVENT,
                                  BTSTACK_WORKER_SIGNAL_WORK) == 37);
    CHECK(test_context.notify_id == BTSTACK_WORKER_EVENT);
    CHECK(test_context.notify_signals == BTSTACK_WORKER_SIGNAL_WORK);
    CHECK(btstack_platform_wait(BTSTACK_WORKER_STACK,
                                BTSTACK_WORKER_SIGNAL_COMMAND_SLOT,
                                BTSTACK_WORKER_WAIT_FOREVER) ==
          BTSTACK_WORKER_SIGNAL_COMMAND_SLOT);
    CHECK(test_context.wait_id == BTSTACK_WORKER_STACK);
    CHECK(test_context.wait_mask == BTSTACK_WORKER_SIGNAL_COMMAND_SLOT);
    CHECK(test_context.wait_timeout_ms == BTSTACK_WORKER_WAIT_FOREVER);
    CHECK(btstack_platform_notify(BTSTACK_WORKER_COUNT,
                                  BTSTACK_WORKER_SIGNAL_WORK) ==
          BTSTACK_STATUS_NOT_READY);
    CHECK(btstack_platform_wait(BTSTACK_WORKER_COUNT,
                                BTSTACK_WORKER_SIGNAL_WORK,
                                1u) == 0u);
    CHECK(btstack_platform_notify((btstack_worker_id_t)-1,
                                  BTSTACK_WORKER_SIGNAL_WORK) ==
          BTSTACK_STATUS_NOT_READY);
    CHECK(btstack_platform_wait((btstack_worker_id_t)-1,
                                BTSTACK_WORKER_SIGNAL_WORK,
                                1u) == 0u);
    CHECK(btstack_platform_post_stack_callback(&callback));
    CHECK(test_context.posted_callback == &callback);
    test_context.posted_callback = &callback;
    CHECK(!btstack_platform_post_stack_callback(NULL));
    CHECK(test_context.posted_callback == &callback);

    enter_before = test_context.enter_count;
    btstack_core_state_set_value(
        BT_XSTACK_REQUEST_SET_PROFILE_MASK, 0x1234u);
    memset(&state, 0, sizeof(state));
    CHECK(btstack_core_copy_state(&state) == 0);
    CHECK(state.profile_mask == 0x1234u);
    CHECK(test_context.enter_count == enter_before + 3u);

    enter_before = test_context.enter_count;
    btstack_property_reset();
    CHECK(btstack_property_set_classic_address(
              property_address, false) == BTSTACK_STATUS_OK);
    CHECK(btstack_property_get_classic_address(property_snapshot));
    CHECK(memcmp(property_snapshot, property_address,
                 sizeof(property_snapshot)) == 0);
    CHECK(test_context.enter_count > enter_before);

    enter_before = test_context.enter_count;
    btstack_event_reset();
    btstack_event_register_app_callback(test_app_event);
    CHECK(btstack_event_post(
              BTSTACK_TRANSPORT_CLASSIC,
              BTSTACK_EVENT_READY,
              NULL,
              0u) == BTSTACK_STATUS_OK);
    CHECK(test_context.app_event_count == 1u);
    CHECK(test_context.enter_count > enter_before);
    CHECK(test_context.critical_depth == 0u);
    CHECK(test_context.enter_count == test_context.exit_count);
    CHECK(test_context.critical_violation_count == 0u);

    active_legacy = *btstack_port_ops_get_ops();
    CHECK(btstack_platform_activate(&other_valid) ==
          BTSTACK_PLATFORM_ERROR_ALREADY_ACTIVE);
    CHECK(btstack_platform_get() == &valid);
    CHECK(memcmp(&active_legacy, btstack_port_ops_get_ops(),
                 sizeof(active_legacy)) == 0);
    bad_version.abi_version++;
    CHECK(btstack_platform_activate(&bad_version) ==
          BTSTACK_PLATFORM_ERROR_ALREADY_ACTIVE);
    CHECK(btstack_platform_get() == &valid);
    CHECK(memcmp(&active_legacy, btstack_port_ops_get_ops(),
                 sizeof(active_legacy)) == 0);

    puts("PASS: BTstack platform descriptor validation and legacy view");
    return 0;
}
