#include "../include/api/bt_xstack.h"
#include "btstack_core_internal.h"
#include "../core/btstack_ring.h"
#include "core/btstack_profile_manager.h"
#include "profiles/btstack_avrcp_cover_art.h"

#include <stddef.h>
#include <string.h>

#include "btstack_runtime.h"
#include "port/btstack_platform.h"

#define BT_XSTACK_API_VERSION "GPCV2178_XSTACK_API_1.0"

static btstack_core_request_t bt_xstack_request_queue_storage[
    BT_XSTACK_REQUEST_QUEUE_LEN];
static btstack_core_event_envelope_t bt_xstack_critical_event_queue_storage[
    BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN];
static btstack_core_event_envelope_t
    bt_xstack_best_effort_event_queue_storage[
        BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN];
static btstack_ring_t bt_xstack_request_queue;
static btstack_ring_t bt_xstack_critical_event_queue;
static btstack_ring_t bt_xstack_best_effort_event_queue;
static bt_xstack_config_t bt_xstack_api_config;
static bt_xstack_cover_art_callback_t bt_xstack_api_cover_art_callback;
static void *bt_xstack_api_cover_art_context;
static volatile uint8_t bt_xstack_api_initialized;
static volatile uint8_t bt_xstack_api_initializing;
static volatile uint8_t bt_xstack_api_failed;
static volatile uint8_t bt_xstack_runtime_initialized;
static uint32_t bt_xstack_api_generation;
static volatile uint32_t bt_xstack_critical_event_drop_count;
static volatile uint32_t bt_xstack_best_effort_event_drop_count;
static volatile uint32_t bt_xstack_api_accepted_request_count;
static volatile uint32_t bt_xstack_api_rejected_request_count;
static volatile uint8_t bt_xstack_pbap_event_space_request_pending;

static void bt_xstack_api_mark_failed(void)
{
    btstack_platform_enter_critical();
    bt_xstack_api_failed = 1u;
    btstack_platform_exit_critical();
}

static int bt_xstack_api_config_equal(const bt_xstack_config_t *config)
{
    return (config->service_callback ==
                bt_xstack_api_config.service_callback) &&
           (config->connection_callback ==
                bt_xstack_api_config.connection_callback) &&
           (config->phone_callback ==
                bt_xstack_api_config.phone_callback) &&
           (config->audio_event_callback ==
                bt_xstack_api_config.audio_event_callback) &&
           (config->phonebook_callback == bt_xstack_api_config.phonebook_callback) &&
           (config->le_callback == bt_xstack_api_config.le_callback) &&
           (config->gatt_callback == bt_xstack_api_config.gatt_callback) &&
           (config->cover_art_callback == bt_xstack_api_config.cover_art_callback) &&
           (config->media_audio_callback == bt_xstack_api_config.media_audio_callback) &&
           (config->call_audio_callback == bt_xstack_api_config.call_audio_callback) &&
           (config->context == bt_xstack_api_config.context) &&
           (config->task_priority ==
                bt_xstack_api_config.task_priority) &&
           (config->task_stack_words ==
                bt_xstack_api_config.task_stack_words);
}

static bt_xstack_result_t bt_xstack_api_copy_string(
    char *destination,
    uint16_t capacity,
    const char *source)
{
    size_t length;

    if ((destination == NULL) || (source == NULL) || (capacity == 0u)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    length = strlen(source);
    if (length >= capacity) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memcpy(destination, source, length + 1u);
    return BT_XSTACK_OK;
}

static bt_xstack_result_t bt_xstack_api_submit(
    const btstack_core_request_t *request)
{
    btstack_core_request_t copy;

    if (request == NULL) {
        btstack_platform_enter_critical();
        bt_xstack_api_rejected_request_count++;
        btstack_platform_exit_critical();
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (!bt_xstack_api_initialized) {
        btstack_platform_enter_critical();
        bt_xstack_api_rejected_request_count++;
        btstack_platform_exit_critical();
        return BT_XSTACK_NOT_INITIALIZED;
    }
    if (bt_xstack_api_failed) {
        btstack_platform_enter_critical();
        bt_xstack_api_rejected_request_count++;
        btstack_platform_exit_critical();
        return BT_XSTACK_NOT_READY;
    }
    if (!btstack_runtime_started()) {
        btstack_platform_enter_critical();
        bt_xstack_api_rejected_request_count++;
        btstack_platform_exit_critical();
        return BT_XSTACK_NOT_READY;
    }

    memcpy(&copy, request, sizeof(copy));
    btstack_platform_enter_critical();
    copy.generation = ++bt_xstack_api_generation;
    if (!btstack_ring_push(&bt_xstack_request_queue, &copy)) {
        bt_xstack_api_rejected_request_count++;
        btstack_platform_exit_critical();
        return BT_XSTACK_QUEUE_FULL;
    }
    bt_xstack_api_accepted_request_count++;
    btstack_platform_exit_critical();
    (void)btstack_platform_notify(
        BTSTACK_WORKER_API, BTSTACK_WORKER_SIGNAL_WORK);
    return BT_XSTACK_OK;
}

static bt_xstack_result_t bt_xstack_api_submit_value(
    bt_xstack_request_id_t id,
    uint32_t value)
{
    btstack_core_request_t request;

    memset(&request, 0, sizeof(request));
    request.id = id;
    request.values[0] = value;
    request.value_count = 1u;
    return bt_xstack_api_submit(&request);
}

static bt_xstack_result_t bt_xstack_api_snapshot(bt_xstack_state_t *state)
{
    if (state == NULL) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_copy_state(state) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static bt_xstack_result_t bt_xstack_api_read_service_snapshot(
    btstack_core_service_snapshot_t *snapshot)
{
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_read_service(snapshot) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static bt_xstack_result_t bt_xstack_api_read_hfp_snapshot(
    btstack_core_hfp_snapshot_t *snapshot)
{
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_read_hfp(snapshot) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static bt_xstack_result_t bt_xstack_api_read_a2dp_snapshot(
    btstack_core_a2dp_snapshot_t *snapshot)
{
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_read_a2dp(snapshot) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static bt_xstack_result_t bt_xstack_api_read_avrcp_snapshot(
    btstack_core_avrcp_snapshot_t *snapshot)
{
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_read_avrcp(snapshot) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static void bt_xstack_api_submit_event_task_request(
    const btstack_core_request_t *request,
    uint8_t count_accepted)
{
    btstack_core_request_t copy;
    bool sent;

    memcpy(&copy, request, sizeof(copy));
    btstack_platform_enter_critical();
    copy.generation = ++bt_xstack_api_generation;
    btstack_platform_exit_critical();
    for (;;) {
        btstack_platform_enter_critical();
        sent = btstack_ring_push(&bt_xstack_request_queue, &copy);
        if (sent && count_accepted) {
            bt_xstack_api_accepted_request_count++;
        }
        btstack_platform_exit_critical();
        if (sent) {
            (void)btstack_platform_notify(
                BTSTACK_WORKER_API, BTSTACK_WORKER_SIGNAL_WORK);
            return;
        }
        (void)btstack_platform_wait(
            BTSTACK_WORKER_EVENT,
            BTSTACK_WORKER_SIGNAL_REQUEST_SPACE,
            BTSTACK_WORKER_WAIT_FOREVER);
    }
}

static void bt_xstack_api_request_pbap_event_space_available(void)
{
    btstack_core_request_t request;

    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE;
    btstack_platform_enter_critical();
    if (bt_xstack_pbap_event_space_request_pending) {
        btstack_platform_exit_critical();
        return;
    }
    bt_xstack_pbap_event_space_request_pending = 1u;
    btstack_platform_exit_critical();
    bt_xstack_api_submit_event_task_request(&request, 0u);
}

static void bt_xstack_event_task(void *context)
{
    btstack_core_event_envelope_t event;
    bool received;

    (void)context;
    for (;;) {
        for (;;) {
            btstack_platform_enter_critical();
            received = btstack_ring_pop(
                &bt_xstack_critical_event_queue, &event);
            btstack_platform_exit_critical();
            if (received) {
                btstack_frontend_deliver_event(&event);
                bt_xstack_api_request_pbap_event_space_available();
                continue;
            }
            btstack_platform_enter_critical();
            received = btstack_ring_pop(
                &bt_xstack_best_effort_event_queue, &event);
            btstack_platform_exit_critical();
            if (!received) {
                break;
            }
            btstack_frontend_deliver_event(&event);
        }
        (void)btstack_platform_wait(
            BTSTACK_WORKER_EVENT,
            BTSTACK_WORKER_SIGNAL_WORK |
                BTSTACK_WORKER_SIGNAL_REQUEST_SPACE,
            BTSTACK_WORKER_WAIT_FOREVER);
    }
}

static int bt_xstack_api_process_request(
    const btstack_core_request_t *request)
{
    int result;

    if (!btstack_runtime_started()) {
        return -1;
    }
    do {
        result = btstack_core_dispatch(request);
        if (result == BT_XSTACK_BACKEND_BUSY) {
            (void)btstack_platform_wait(
                BTSTACK_WORKER_API,
                BTSTACK_WORKER_SIGNAL_COMMAND_SLOT |
                    BTSTACK_WORKER_SIGNAL_WORK,
                BTSTACK_WORKER_WAIT_FOREVER);
        }
    } while (result == BT_XSTACK_BACKEND_BUSY);
    return result;
}

static void bt_xstack_api_task(void *context)
{
    btstack_core_request_t request;
    bool received;

    (void)context;
    for (;;) {
        for (;;) {
            btstack_platform_enter_critical();
            received = btstack_ring_pop(
                &bt_xstack_request_queue, &request);
            btstack_platform_exit_critical();
            if (!received) {
                break;
            }
            (void)btstack_platform_notify(
                BTSTACK_WORKER_EVENT,
                BTSTACK_WORKER_SIGNAL_REQUEST_SPACE);
            if (!bt_xstack_api_failed) {
                (void)bt_xstack_api_process_request(&request);
            }
        }
        (void)btstack_platform_wait(
            BTSTACK_WORKER_API,
            BTSTACK_WORKER_SIGNAL_WORK |
                BTSTACK_WORKER_SIGNAL_COMMAND_SLOT,
            BTSTACK_WORKER_WAIT_FOREVER);
    }
}

bt_xstack_result_t bt_xstack_init(const bt_xstack_config_t *config)
{
    btstack_worker_config_t api_worker_config;
    btstack_worker_config_t event_worker_config;
    uint8_t runtime_initialized;

    if (config == NULL) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (btstack_platform_get() == NULL) {
        if (btstack_platform_activate(btstack_platform_default()) !=
                BTSTACK_PLATFORM_OK) {
            return BT_XSTACK_INTERNAL_ERROR;
        }
    }
    api_worker_config.priority = config->task_priority;
    api_worker_config.stack_words = (config->task_stack_words == 0u) ?
        BT_XSTACK_API_TASK_STACK_WORDS : config->task_stack_words;
    event_worker_config.priority = config->task_priority;
    event_worker_config.stack_words = BT_XSTACK_EVENT_TASK_STACK_WORDS;
    if ((btstack_platform_worker_validate(
            BTSTACK_WORKER_API, &api_worker_config) != 0) ||
        (btstack_platform_worker_validate(
            BTSTACK_WORKER_EVENT, &event_worker_config) != 0)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }

    btstack_platform_enter_critical();
    if (bt_xstack_api_failed) {
        btstack_platform_exit_critical();
        return BT_XSTACK_INTERNAL_ERROR;
    }
    if (bt_xstack_api_initialized) {
        int equal = bt_xstack_api_config_equal(config);
        btstack_platform_exit_critical();
        return equal ? BT_XSTACK_OK :
            BT_XSTACK_CONFIGURATION_CONFLICT;
    }
    if (bt_xstack_api_initializing) {
        int equal = bt_xstack_api_config_equal(config);
        btstack_platform_exit_critical();
        return equal ? BT_XSTACK_NOT_READY :
            BT_XSTACK_CONFIGURATION_CONFLICT;
    }
    bt_xstack_api_initializing = 1u;
    memcpy(&bt_xstack_api_config, config, sizeof(*config));
    bt_xstack_api_cover_art_callback = config->cover_art_callback;
    bt_xstack_api_cover_art_context = config->context;
    bt_xstack_api_accepted_request_count = 0u;
    bt_xstack_api_rejected_request_count = 0u;
    bt_xstack_critical_event_drop_count = 0u;
    bt_xstack_best_effort_event_drop_count = 0u;
    bt_xstack_pbap_event_space_request_pending = 0u;
    btstack_platform_exit_critical();

    btstack_ring_init(
        &bt_xstack_request_queue,
        bt_xstack_request_queue_storage,
        sizeof(bt_xstack_request_queue_storage[0]),
        BT_XSTACK_REQUEST_QUEUE_LEN);
    btstack_ring_init(
        &bt_xstack_critical_event_queue,
        bt_xstack_critical_event_queue_storage,
        sizeof(bt_xstack_critical_event_queue_storage[0]),
        BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN);
    btstack_ring_init(
        &bt_xstack_best_effort_event_queue,
        bt_xstack_best_effort_event_queue_storage,
        sizeof(bt_xstack_best_effort_event_queue_storage[0]),
        BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN);
    btstack_platform_enter_critical();
    runtime_initialized = bt_xstack_runtime_initialized;
    btstack_platform_exit_critical();
    if (!runtime_initialized) {
        int runtime_status = btstack_runtime_init();
        if (runtime_status == BTSTACK_STATUS_BUSY) {
            btstack_platform_enter_critical();
            bt_xstack_api_initializing = 0u;
            btstack_platform_exit_critical();
            return BT_XSTACK_NOT_READY;
        }
        if (runtime_status != BTSTACK_STATUS_OK) {
            btstack_platform_enter_critical();
            bt_xstack_api_failed = 1u;
            bt_xstack_api_initializing = 0u;
            btstack_platform_exit_critical();
            return BT_XSTACK_INTERNAL_ERROR;
        }
        btstack_platform_enter_critical();
        bt_xstack_runtime_initialized = 1u;
        btstack_platform_exit_critical();
    }

    if (btstack_platform_worker_start(
            BTSTACK_WORKER_API,
            bt_xstack_api_task,
            NULL,
            &api_worker_config) != 0) {
        btstack_platform_enter_critical();
        bt_xstack_api_initializing = 0u;
        btstack_platform_exit_critical();
        return BT_XSTACK_INTERNAL_ERROR;
    }
    if (btstack_platform_worker_start(
            BTSTACK_WORKER_EVENT,
            bt_xstack_event_task,
            NULL,
            &event_worker_config) != 0) {
        (void)btstack_platform_worker_stop(BTSTACK_WORKER_API);
        btstack_platform_enter_critical();
        bt_xstack_api_initializing = 0u;
        btstack_platform_exit_critical();
        return BT_XSTACK_INTERNAL_ERROR;
    }

    btstack_platform_enter_critical();
    bt_xstack_api_initialized = 1u;
    bt_xstack_api_initializing = 0u;
    btstack_platform_exit_critical();
    btstack_avrcp_cover_art_set_callback_enabled(config->cover_art_callback != NULL);
    return BT_XSTACK_OK;
}

bool bt_xstack_is_ready(void)
{
    return bt_xstack_api_initialized && !bt_xstack_api_failed;
}

bt_xstack_result_t bt_xstack_enable(void)
{
    int runtime_status;

    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    if (bt_xstack_api_failed) {
        return BT_XSTACK_NOT_READY;
    }
    if (!btstack_runtime_started()) {
        runtime_status = btstack_runtime_start();
        if (runtime_status == BTSTACK_STATUS_BUSY) {
            return BT_XSTACK_NOT_READY;
        }
        if (runtime_status != BTSTACK_STATUS_OK) {
            bt_xstack_api_mark_failed();
            return BT_XSTACK_INTERNAL_ERROR;
        }
    }
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_POWER, 1u);
}

bt_xstack_result_t bt_xstack_disable(void)
{
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    if (bt_xstack_api_failed) {
        return BT_XSTACK_NOT_READY;
    }
    if (!btstack_runtime_started()) {
        return BT_XSTACK_OK;
    }
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_POWER, 0u);
}

bt_xstack_result_t bt_xstack_get_state(bt_xstack_state_t *state)
{
    return bt_xstack_api_snapshot(state);
}

bt_xstack_result_t bt_xstack_get_diagnostics(
    bt_xstack_diagnostics_t *diagnostics)
{
    btstack_core_backend_diagnostics_t backend;
    btstack_runtime_diagnostics_t run_loop;

    if (diagnostics == NULL) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    memset(&backend, 0, sizeof(backend));
    memset(&run_loop, 0, sizeof(run_loop));
    btstack_core_dispatcher_get_diagnostics(&backend);
    btstack_runtime_get_diagnostics(&run_loop);
    btstack_platform_enter_critical();
    diagnostics->accepted_requests =
        bt_xstack_api_accepted_request_count;
    diagnostics->rejected_requests =
        bt_xstack_api_rejected_request_count;
    diagnostics->critical_event_drops =
        bt_xstack_critical_event_drop_count;
    diagnostics->best_effort_event_drops =
        bt_xstack_best_effort_event_drop_count;
    btstack_platform_exit_critical();
    diagnostics->stack_commands = backend.stack_commands;
    diagnostics->stack_command_failures =
        backend.stack_command_failures;
    diagnostics->command_slots_in_use =
        backend.command_slots_in_use;
    diagnostics->command_slots_max = backend.command_slots_max;
    diagnostics->run_loop_ingress_depth = run_loop.current_depth;
    diagnostics->run_loop_ingress_max = run_loop.max_depth;
    diagnostics->run_loop_ingress_full = run_loop.full_count;
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_get_version(char *version, uint16_t capacity)
{
    return bt_xstack_api_copy_string(
        version, capacity, BT_XSTACK_API_VERSION);
}

bt_xstack_result_t bt_xstack_get_profile_mask(uint16_t *profile_mask)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result;
    if (profile_mask == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_service_snapshot(&state);
    if (result == BT_XSTACK_OK) *profile_mask = state.profile_mask;
    return result;
}

bt_xstack_result_t bt_xstack_set_profile_mask(uint16_t profile_mask)
{
    if (profile_mask != BT_XSTACK_PROFILE_MASK_SUPPORTED) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    return bt_xstack_api_submit_value(
        BT_XSTACK_REQUEST_SET_PROFILE_MASK, profile_mask);
}

bt_xstack_result_t bt_xstack_get_local_address(bt_xstack_addr_t *address)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result;
    if (address == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_service_snapshot(&state);
    if (result == BT_XSTACK_OK) memcpy(address, &state.local_address, sizeof(*address));
    return result;
}

bt_xstack_result_t bt_xstack_get_local_name(char *name, uint16_t capacity)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result =
        bt_xstack_api_read_service_snapshot(&state);
    if (result != BT_XSTACK_OK) return result;
    return bt_xstack_api_copy_string(name, capacity, state.local_name);
}

bt_xstack_result_t bt_xstack_set_local_name(
    const char *name,
    bool append_address_suffix)
{
    btstack_core_request_t request;
    size_t length;
    if (name == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    length = strlen(name);
    if ((length == 0u) || (length > 31u) ||
        (append_address_suffix && (length > 26u))) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_SET_NAME;
    request.values[0] = append_address_suffix ? 1u : 0u;
    request.value_count = 1u;
    memcpy(request.text, name, length + 1u);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_get_pin(char *pin, uint16_t capacity)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result =
        bt_xstack_api_read_service_snapshot(&state);
    if (result != BT_XSTACK_OK) return result;
    return bt_xstack_api_copy_string(pin, capacity, state.pin);
}

bt_xstack_result_t bt_xstack_set_pin(const char *pin)
{
    btstack_core_request_t request;
    size_t length;
    if (pin == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    length = strlen(pin);
    if ((length < 4u) || (length > BT_XSTACK_PIN_MAX)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_SET_PIN;
    memcpy(request.text, pin, length + 1u);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_get_ssp_mode(uint8_t *mode)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result;
    if (mode == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_service_snapshot(&state);
    if (result == BT_XSTACK_OK) *mode = state.ssp_mode;
    return result;
}

bt_xstack_result_t bt_xstack_set_ssp_mode(uint8_t mode)
{
    if (mode > 3u) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_SSP, mode);
}

bt_xstack_result_t bt_xstack_pairing_confirm(
    const bt_xstack_addr_t *address,
    bool accept)
{
    btstack_core_request_t request;
    if (address == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PAIR_CONFIRM;
    memcpy(&request.address, address, sizeof(*address));
    request.has_address = 1u;
    request.values[0] = accept ? 1u : 0u;
    request.value_count = 1u;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_set_discoverable(bool enabled)
{
    return bt_xstack_api_submit_value(
        BT_XSTACK_REQUEST_SET_DISCOVERABLE, enabled ? 1u : 0u);
}

bt_xstack_result_t bt_xstack_set_connectable(bool enabled)
{
    return bt_xstack_api_submit_value(
        BT_XSTACK_REQUEST_SET_CONNECTABLE, enabled ? 1u : 0u);
}

bt_xstack_result_t bt_xstack_get_class_of_device(uint32_t *class_of_device)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result;
    if (class_of_device == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_service_snapshot(&state);
    if (result == BT_XSTACK_OK) *class_of_device = state.class_of_device;
    return result;
}

bt_xstack_result_t bt_xstack_set_class_of_device(uint32_t class_of_device)
{
    if (class_of_device > 0xffffffu) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_COD, class_of_device);
}

bt_xstack_result_t bt_xstack_get_paired_device_count(uint8_t *count)
{
    if (count == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_get_peer_count(count) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

bt_xstack_result_t bt_xstack_get_paired_device(
    uint8_t index,
    bt_xstack_paired_device_t *device)
{
    if ((index == 0u) || (index > BT_XSTACK_PAIRED_DEVICE_MAX) ||
        (device == NULL)) return BT_XSTACK_INVALID_ARGUMENT;
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_get_peer(index - 1u, device) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

bt_xstack_result_t bt_xstack_clear_paired_device_by_index(uint8_t index)
{
    if ((index == 0u) || (index > BT_XSTACK_PAIRED_DEVICE_MAX)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_CLEAR_PEER_INDEX, index);
}

bt_xstack_result_t bt_xstack_clear_paired_device_by_address(
    const bt_xstack_addr_t *address)
{
    btstack_core_request_t request;
    if (address == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS;
    memcpy(&request.address, address, sizeof(*address));
    request.has_address = 1u;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_clear_paired_devices(void)
{
    btstack_core_request_t request;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_CLEAR_PEERS;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_get_tdl_mode(uint8_t *mode)
{
    btstack_core_service_snapshot_t state;
    bt_xstack_result_t result;
    if (mode == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_service_snapshot(&state);
    if (result == BT_XSTACK_OK) *mode = state.tdl_mode;
    return result;
}

bt_xstack_result_t bt_xstack_set_tdl_mode(uint8_t mode)
{
    if (mode > BT_XSTACK_PAIRED_DEVICE_MAX) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_TDL, mode);
}

bt_xstack_result_t bt_xstack_scan_start(
    uint8_t period_seconds,
    const char *name_filter)
{
    btstack_core_request_t request;
    size_t length = 0u;
    if ((period_seconds == 0u) || (period_seconds > 48u)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (name_filter != NULL) {
        length = strlen(name_filter);
        if (length >= BT_XSTACK_TEXT_MAX) return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_SCAN_START;
    request.values[0] = period_seconds;
    request.value_count = 1u;
    if (length != 0u) memcpy(request.text, name_filter, length + 1u);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_scan_stop(void)
{
    btstack_core_request_t request;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_SCAN_STOP;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_disconnect_all(void)
{
    btstack_core_request_t request;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_DISCONNECT_ALL;
    return bt_xstack_api_submit(&request);
}

static int bt_xstack_api_profile_valid(bt_xstack_profile_t profile)
{
    return (profile == BT_XSTACK_PROFILE_HFP_HF) ||
           (profile == BT_XSTACK_PROFILE_A2DP_SINK) ||
           (profile == BT_XSTACK_PROFILE_AVRCP);
}

bt_xstack_result_t bt_xstack_connect(
    const bt_xstack_addr_t *address,
    bt_xstack_profile_t profile)
{
    btstack_core_request_t request;
    if ((address == NULL) || !bt_xstack_api_profile_valid(profile)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_CONNECT;
    request.profile = profile;
    memcpy(&request.address, address, sizeof(*address));
    request.has_address = 1u;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_connect_last(bt_xstack_profile_t profile)
{
    btstack_core_request_t request;
    if (!bt_xstack_api_profile_valid(profile)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_CONNECT_LAST;
    request.profile = profile;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_disconnect(bt_xstack_profile_t profile)
{
    btstack_core_request_t request;
    if (!bt_xstack_api_profile_valid(profile)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_DISCONNECT_PROFILE;
    request.profile = profile;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_get_connection_state(
    bt_xstack_profile_t profile,
    bt_xstack_connection_state_t *connection_state)
{
    bt_xstack_result_t result;
    uint8_t profile_state;
    if ((connection_state == NULL) || !bt_xstack_api_profile_valid(profile)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (profile == BT_XSTACK_PROFILE_HFP_HF) {
        btstack_core_hfp_snapshot_t state;
        result = bt_xstack_api_read_hfp_snapshot(&state);
        if (result != BT_XSTACK_OK) return result;
        profile_state = state.state;
    } else if (profile == BT_XSTACK_PROFILE_A2DP_SINK) {
        btstack_core_a2dp_snapshot_t state;
        result = bt_xstack_api_read_a2dp_snapshot(&state);
        if (result != BT_XSTACK_OK) return result;
        profile_state = state.state;
    } else {
        btstack_core_avrcp_snapshot_t state;
        result = bt_xstack_api_read_avrcp_snapshot(&state);
        if (result != BT_XSTACK_OK) return result;
        profile_state = state.state;
    }
    *connection_state = (profile_state == 2u) ? BT_XSTACK_CONNECTION_CONNECTING :
        ((profile_state >= 3u) ? BT_XSTACK_CONNECTION_CONNECTED :
            BT_XSTACK_CONNECTION_DISCONNECTED);
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_get_hfp_config(uint8_t *config)
{
    btstack_core_hfp_snapshot_t state; bt_xstack_result_t result;
    if (config == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_hfp_snapshot(&state); if (result == BT_XSTACK_OK) *config = state.config; return result;
}

bt_xstack_result_t bt_xstack_set_hfp_config(uint8_t config)
{
    if (config > 7u) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_HFP_CONFIG, config);
}

bt_xstack_result_t bt_xstack_get_hfp_state(bt_xstack_hfp_state_t *hfp_state)
{
    btstack_core_hfp_snapshot_t state; bt_xstack_result_t result;
    if (hfp_state == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    result = bt_xstack_api_read_hfp_snapshot(&state); if (result == BT_XSTACK_OK) *hfp_state = (bt_xstack_hfp_state_t)state.state; return result;
}

bt_xstack_result_t bt_xstack_hfp_dial(const char *number)
{
    btstack_core_request_t request; size_t length;
    if (number == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    length = strlen(number); if ((length == 0u) || (length >= BT_XSTACK_TEXT_MAX)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request)); request.id = BT_XSTACK_REQUEST_HFP_DIAL; memcpy(request.text, number, length + 1u); return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_hfp_redial(void) { btstack_core_request_t r; memset(&r,0,sizeof(r)); r.id=BT_XSTACK_REQUEST_HFP_REDIAL; return bt_xstack_api_submit(&r); }
bt_xstack_result_t bt_xstack_hfp_send_dtmf(char code) { btstack_core_request_t r; if (!(((code>='0')&&(code<='9'))||(code=='*')||(code=='#'))) return BT_XSTACK_INVALID_ARGUMENT; memset(&r,0,sizeof(r)); r.id=BT_XSTACK_REQUEST_HFP_DTMF; r.text[0]=code; return bt_xstack_api_submit(&r); }

static bt_xstack_result_t bt_xstack_api_hfp_control(uint32_t control, uint32_t value)
{
    btstack_core_request_t request; memset(&request,0,sizeof(request)); request.id=BT_XSTACK_REQUEST_HFP_CONTROL; request.values[0]=control; request.values[1]=value; request.value_count=2u; return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_hfp_answer(void) { return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_ANSWER,0u); }
bt_xstack_result_t bt_xstack_hfp_hang_up(void) { return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_HANG_UP,0u); }
bt_xstack_result_t bt_xstack_hfp_multiparty_control(bt_xstack_hfp_multiparty_action_t action) { if (action>BT_XSTACK_HFP_MULTIPARTY_JOIN_CALLS) return BT_XSTACK_INVALID_ARGUMENT; return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_MULTIPARTY,(uint32_t)action); }
bt_xstack_result_t bt_xstack_hfp_toggle_audio(void) { return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO,0u); }
bt_xstack_result_t bt_xstack_hfp_set_audio_connected(bool connected) { return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_SET_AUDIO,connected?1u:0u); }
bt_xstack_result_t bt_xstack_hfp_set_voice_recognition(bool enabled) { return bt_xstack_api_hfp_control(BT_XSTACK_HFP_CONTROL_VOICE_RECOGNITION,enabled?1u:0u); }

bt_xstack_result_t bt_xstack_get_microphone_mute(bool *muted) { btstack_core_hfp_snapshot_t s; bt_xstack_result_t r; if(muted==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_hfp_snapshot(&s);if(r==BT_XSTACK_OK)*muted=s.microphone_muted!=0u;return r; }
bt_xstack_result_t bt_xstack_set_microphone_mute(bool muted) { return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE,muted?1u:0u); }
bt_xstack_result_t bt_xstack_get_speaker_volume(uint8_t *volume) { btstack_core_hfp_snapshot_t s; bt_xstack_result_t r;if(volume==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_hfp_snapshot(&s);if(r==BT_XSTACK_OK)*volume=s.speaker_volume;return r; }
bt_xstack_result_t bt_xstack_set_speaker_volume(uint8_t volume) { if(volume>15u)return BT_XSTACK_INVALID_ARGUMENT;return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME,volume); }
bt_xstack_result_t bt_xstack_get_a2dp_state(bt_xstack_a2dp_state_t *a2dp_state) { btstack_core_a2dp_snapshot_t s;bt_xstack_result_t r;if(a2dp_state==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_a2dp_snapshot(&s);if(r==BT_XSTACK_OK)*a2dp_state=(bt_xstack_a2dp_state_t)s.state;return r; }
bt_xstack_result_t bt_xstack_get_a2dp_mute(bool *muted) { btstack_core_a2dp_snapshot_t s;bt_xstack_result_t r;if(muted==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_a2dp_snapshot(&s);if(r==BT_XSTACK_OK)*muted=s.muted!=0u;return r; }
bt_xstack_result_t bt_xstack_set_a2dp_mute(bool muted) { return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_A2DP_MUTE,muted?1u:0u); }
bt_xstack_result_t bt_xstack_get_avrcp_state(bt_xstack_avrcp_state_t *avrcp_state) { btstack_core_avrcp_snapshot_t s;bt_xstack_result_t r;if(avrcp_state==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_avrcp_snapshot(&s);if(r==BT_XSTACK_OK)*avrcp_state=(bt_xstack_avrcp_state_t)s.state;return r; }
bt_xstack_result_t bt_xstack_get_avrcp_play_state(bt_xstack_play_state_t *play_state) { btstack_core_avrcp_snapshot_t s;bt_xstack_result_t r;if(play_state==NULL)return BT_XSTACK_INVALID_ARGUMENT;r=bt_xstack_api_read_avrcp_snapshot(&s);if(r==BT_XSTACK_OK)*play_state=(bt_xstack_play_state_t)s.play_state;return r; }

static bt_xstack_result_t bt_xstack_api_avrcp_control(uint32_t control) { return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_AVRCP_CONTROL,control); }
bt_xstack_result_t bt_xstack_avrcp_play_pause(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE); }
bt_xstack_result_t bt_xstack_avrcp_play(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_PLAY); }
bt_xstack_result_t bt_xstack_avrcp_pause(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_PAUSE); }
bt_xstack_result_t bt_xstack_avrcp_stop(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_STOP); }
bt_xstack_result_t bt_xstack_avrcp_next(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_NEXT); }
bt_xstack_result_t bt_xstack_avrcp_previous(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_PREVIOUS); }
bt_xstack_result_t bt_xstack_avrcp_fast_forward(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_FAST_FORWARD); }
bt_xstack_result_t bt_xstack_avrcp_rewind(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_REWIND); }
bt_xstack_result_t bt_xstack_avrcp_set_repeat(bool enabled) { return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_REPEAT,enabled?1u:0u); }
bt_xstack_result_t bt_xstack_avrcp_set_shuffle(bool enabled) { return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_SET_SHUFFLE,enabled?1u:0u); }
bt_xstack_result_t bt_xstack_avrcp_volume_up(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_VOLUME_UP); }
bt_xstack_result_t bt_xstack_avrcp_volume_down(void) { return bt_xstack_api_avrcp_control(BT_XSTACK_AVRCP_CONTROL_VOLUME_DOWN); }

bt_xstack_result_t bt_xstack_phonebook_sync(
    const bt_xstack_addr_t *address,
    bt_xstack_phonebook_category_mask_t categories)
{
    btstack_core_request_t request;

    if ((address == NULL) || (categories == 0u) ||
        ((categories & (bt_xstack_phonebook_category_mask_t)
            ~BT_XSTACK_PHONEBOOK_ALL) != 0u)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_SYNC;
    memcpy(&request.address, address, sizeof(*address));
    request.has_address = 1u;
    request.values[0] = categories;
    request.value_count = 1u;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_phonebook_cancel(void)
{
    btstack_core_request_t request;

    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_CANCEL;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_phonebook_authenticate(const char *password)
{
    btstack_core_request_t request;
    size_t length;

    if (password == NULL) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    length = strlen(password);
    if (length > BT_XSTACK_PHONEBOOK_PASSWORD_MAX) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_AUTHENTICATE;
    memcpy(request.text, password, length + 1u);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_cover_art_set_mode(bt_xstack_cover_art_mode_t mode)
{
    btstack_core_request_t request;

    if (mode != BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL && mode != BT_XSTACK_COVER_ART_MODE_IMAGE) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_COVER_ART_SET_MODE;
    request.cover_art.mode = mode;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_cover_art_register_callback(
    bt_xstack_cover_art_callback_t callback, void *context)
{
    btstack_platform_enter_critical();
    if (!bt_xstack_api_initialized) {
        btstack_platform_exit_critical();
        return BT_XSTACK_NOT_INITIALIZED;
    }
    bt_xstack_api_cover_art_callback = callback;
    bt_xstack_api_cover_art_context = context;
    btstack_platform_exit_critical();

    btstack_avrcp_cover_art_set_callback_enabled(callback != NULL);
    return BT_XSTACK_OK;
}

bt_xstack_result_t bt_xstack_cover_art_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor)
{
    btstack_core_request_t request;

    if (descriptor == NULL || descriptor->encoding <= BT_XSTACK_COVER_ART_ENCODING_UNKNOWN ||
        descriptor->encoding > BT_XSTACK_COVER_ART_ENCODING_WBMP || descriptor->width == 0u || descriptor->height == 0u ||
        (descriptor->transformation & (uint8_t)~BT_XSTACK_COVER_ART_TRANSFORMATION_ALL) != 0u) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE;
    request.cover_art.descriptor = *descriptor;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_phonebook_get_state(
    bt_xstack_phonebook_state_t *state)
{
    if (state == NULL) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    if (!bt_xstack_api_initialized) {
        return BT_XSTACK_NOT_INITIALIZED;
    }
    return (btstack_core_state_read_pbap(state) == 0) ?
        BT_XSTACK_OK : BT_XSTACK_NOT_READY;
}

static int bt_xstack_api_uuid_valid(const bt_xstack_uuid_t *uuid)
{
    return (uuid != NULL) && ((uuid->type == BT_XSTACK_UUID_TYPE_16) ||
        (uuid->type == BT_XSTACK_UUID_TYPE_128));
}

static bt_xstack_result_t bt_xstack_api_submit_empty(bt_xstack_request_id_t id)
{
    btstack_core_request_t request;
    memset(&request, 0, sizeof(request));
    request.id = id;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_le_adv_configure(const bt_xstack_le_adv_config_t *config)
{
    btstack_core_request_t request;
    if ((config == NULL) || (config->data_len > BT_XSTACK_LE_ADV_DATA_MAX) ||
        (config->scan_response_len > BT_XSTACK_LE_ADV_DATA_MAX) ||
        (config->interval_min == 0u) || (config->interval_min > config->interval_max)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_LE_ADV_CONFIGURE;
    request.le_adv = *config;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_le_adv_start(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_ADV_START); }
bt_xstack_result_t bt_xstack_le_adv_stop(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_ADV_STOP); }

bt_xstack_result_t bt_xstack_le_scan_start(const bt_xstack_le_scan_config_t *config)
{
    btstack_core_request_t request;
    if ((config == NULL) || (config->interval == 0u) ||
        (config->window == 0u) || (config->window > config->interval)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_LE_SCAN_START;
    request.le_scan = *config;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_le_scan_stop(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_SCAN_STOP); }

bt_xstack_result_t bt_xstack_le_connect(uint8_t address_type,
    const bt_xstack_addr_t *address)
{
    btstack_core_request_t request;
    if ((address == NULL) || (address_type > 1u)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_LE_CONNECT;
    request.le_connect.address_type = address_type;
    request.le_connect.address = *address;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_le_connect_cancel(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_CONNECT_CANCEL); }
bt_xstack_result_t bt_xstack_le_disconnect(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_DISCONNECT); }
bt_xstack_result_t bt_xstack_le_pair(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_PAIR); }
bt_xstack_result_t bt_xstack_le_pairing_confirm(bool accept)
{ return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_LE_PAIRING_CONFIRM, accept ? 1u : 0u); }
bt_xstack_result_t bt_xstack_le_passkey_reply(uint32_t passkey)
{
    if (passkey > 999999u) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_LE_PASSKEY_REPLY, passkey);
}
bt_xstack_result_t bt_xstack_le_bond_delete(uint16_t index)
{
    if (index >= BTSTACK_LE_BOND_MAX) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_submit_value(BT_XSTACK_REQUEST_LE_BOND_DELETE, index);
}
bt_xstack_result_t bt_xstack_le_bond_delete_all(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_LE_BOND_DELETE_ALL); }

static bt_xstack_result_t bt_xstack_api_gatt_register(bt_xstack_request_id_t id,
    uint16_t parent_handle, const bt_xstack_uuid_t *uuid,
    const bt_xstack_gatt_attribute_config_t *config)
{
    btstack_core_request_t request;
    if (!bt_xstack_api_uuid_valid(uuid)) return BT_XSTACK_INVALID_ARGUMENT;
    if ((config != NULL) && ((!bt_xstack_api_uuid_valid(&config->uuid)) ||
        (config->max_len == 0u) || (config->max_len > BT_XSTACK_GATT_VALUE_MAX) ||
        (config->read_security > BT_XSTACK_GATT_SECURITY_AUTHENTICATED) ||
        (config->write_security > BT_XSTACK_GATT_SECURITY_AUTHENTICATED))) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = id;
    request.gatt_registration.parent_handle = parent_handle;
    request.gatt_registration.uuid = *uuid;
    if (config != NULL) request.gatt_registration.config = *config;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_server_add_service(const bt_xstack_uuid_t *uuid)
{ return bt_xstack_api_gatt_register(BT_XSTACK_REQUEST_GATT_SERVER_ADD_SERVICE, 0u, uuid, NULL); }

bt_xstack_result_t bt_xstack_gatt_server_add_characteristic(uint16_t service_handle,
    const bt_xstack_gatt_attribute_config_t *config)
{
    if (config == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_gatt_register(BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC,
        service_handle, &config->uuid, config);
}

bt_xstack_result_t bt_xstack_gatt_server_add_descriptor(uint16_t value_handle,
    const bt_xstack_gatt_attribute_config_t *config)
{
    if (config == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    return bt_xstack_api_gatt_register(BT_XSTACK_REQUEST_GATT_SERVER_ADD_DESCRIPTOR,
        value_handle, &config->uuid, config);
}

bt_xstack_result_t bt_xstack_gatt_server_build(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_GATT_SERVER_BUILD); }

static bt_xstack_result_t bt_xstack_api_gatt_value(bt_xstack_request_id_t id,
    uint16_t handle, const uint8_t *value, uint16_t value_len, uint8_t flags)
{
    btstack_core_request_t request;
    if ((handle == 0u) || ((value == NULL) && (value_len != 0u)) ||
        (value_len > BT_XSTACK_GATT_VALUE_MAX)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = id;
    request.gatt.handle = handle;
    request.gatt.value_len = value_len;
    request.gatt.flags = flags;
    if (value_len != 0u) memcpy(request.gatt.value, value, value_len);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_server_set_value(uint16_t handle,
    const uint8_t *value, uint16_t value_len)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE, handle, value, value_len, 0u); }
bt_xstack_result_t bt_xstack_gatt_server_get_value(uint16_t handle)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_SERVER_GET_VALUE, handle, NULL, 0u, 0u); }
bt_xstack_result_t bt_xstack_gatt_server_notify(uint16_t handle,
    const uint8_t *value, uint16_t value_len)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_SERVER_NOTIFY, handle, value, value_len, 0u); }
bt_xstack_result_t bt_xstack_gatt_server_indicate(uint16_t handle,
    const uint8_t *value, uint16_t value_len)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_SERVER_INDICATE, handle, value, value_len, 0u); }

bt_xstack_result_t bt_xstack_gatt_client_discover_services(const bt_xstack_uuid_t *uuid)
{
    btstack_core_request_t request;
    if ((uuid != NULL) && !bt_xstack_api_uuid_valid(uuid)) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_SERVICES;
    if (uuid != NULL) { request.gatt.uuid = *uuid; request.gatt.flags = 1u; }
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_client_discover_characteristics(uint16_t start_handle,
    uint16_t end_handle, const bt_xstack_uuid_t *uuid)
{
    btstack_core_request_t request;
    if ((start_handle == 0u) || (end_handle < start_handle) ||
        ((uuid != NULL) && !bt_xstack_api_uuid_valid(uuid))) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_CHARACTERISTICS;
    request.gatt.handle = start_handle;
    request.gatt.end_handle = end_handle;
    if (uuid != NULL) { request.gatt.uuid = *uuid; request.gatt.flags = 1u; }
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_client_discover_descriptors(
    const bt_xstack_gatt_characteristic_t *characteristic)
{
    btstack_core_request_t request;
    if (characteristic == NULL) return BT_XSTACK_INVALID_ARGUMENT;
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_DESCRIPTORS;
    request.gatt.characteristic = *characteristic;
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_client_read(uint16_t handle)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_CLIENT_READ, handle, NULL, 0u, 0u); }
bt_xstack_result_t bt_xstack_gatt_client_write(uint16_t handle,
    const uint8_t *value, uint16_t value_len, bool without_response)
{ return bt_xstack_api_gatt_value(BT_XSTACK_REQUEST_GATT_CLIENT_WRITE, handle,
    value, value_len, without_response ? 1u : 0u); }

bt_xstack_result_t bt_xstack_gatt_client_subscribe(
    const bt_xstack_gatt_characteristic_t *characteristic,
    bool notifications, bool indications)
{
    btstack_core_request_t request;
    if ((characteristic == NULL) || (!notifications && !indications)) {
        return BT_XSTACK_INVALID_ARGUMENT;
    }
    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_GATT_CLIENT_SUBSCRIBE;
    request.gatt.characteristic = *characteristic;
    request.gatt.flags = (notifications ? 1u : 0u) | (indications ? 2u : 0u);
    return bt_xstack_api_submit(&request);
}

bt_xstack_result_t bt_xstack_gatt_client_unsubscribe(void)
{ return bt_xstack_api_submit_empty(BT_XSTACK_REQUEST_GATT_CLIENT_UNSUBSCRIBE); }

void btstack_core_command_slot_released(void)
{
    (void)btstack_platform_notify(
        BTSTACK_WORKER_API, BTSTACK_WORKER_SIGNAL_COMMAND_SLOT);
}

void btstack_core_request_completed(
    const btstack_core_request_t *request,
    int status)
{
    (void)request;
    (void)status;
}

void btstack_core_pbap_event_space_request_completed(void)
{
    btstack_platform_enter_critical();
    bt_xstack_pbap_event_space_request_pending = 0u;
    btstack_platform_exit_critical();
}

static bool bt_xstack_api_is_control_event(
    const btstack_core_event_envelope_t *event)
{
    switch (event->domain) {
    case BT_XSTACK_EVENT_DOMAIN_SERVICE:
        switch (event->data.service.id) {
        case BT_XSTACK_SERVICE_EVENT_DEVICE_STATE:
        case BT_XSTACK_SERVICE_EVENT_POWER_STATE:
        case BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE:
        case BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST:
        case BT_XSTACK_SERVICE_EVENT_PAIRED:
            return true;
        default:
            return false;
        }
    case BT_XSTACK_EVENT_DOMAIN_CONNECTION:
        switch (event->data.connection.state) {
        case BT_XSTACK_CONNECTION_DISCONNECTED:
        case BT_XSTACK_CONNECTION_CONNECTING:
        case BT_XSTACK_CONNECTION_CONNECTED:
        case BT_XSTACK_CONNECTION_DISCONNECTING:
            return true;
        default:
            return false;
        }
    case BT_XSTACK_EVENT_DOMAIN_PHONE:
        switch (event->data.phone.id) {
        case BT_XSTACK_PHONE_EVENT_STATE:
        case BT_XSTACK_PHONE_EVENT_DEVICE:
        case BT_XSTACK_PHONE_EVENT_AUDIO:
        case BT_XSTACK_PHONE_EVENT_LOST:
            return true;
        default:
            return false;
        }
    case BT_XSTACK_EVENT_DOMAIN_AUDIO:
        switch (event->data.audio.id) {
        case BT_XSTACK_AUDIO_EVENT_A2DP_STATE:
        case BT_XSTACK_AUDIO_EVENT_A2DP_DEVICE:
        case BT_XSTACK_AUDIO_EVENT_A2DP_LOST:
        case BT_XSTACK_AUDIO_EVENT_PCM_CONFIG:
        case BT_XSTACK_AUDIO_EVENT_AVRCP_STATE:
            return true;
        default:
            return false;
        }
    case BT_XSTACK_EVENT_DOMAIN_PHONEBOOK:
        switch (event->data.phonebook.id) {
        case BT_XSTACK_PHONEBOOK_EVENT_STARTED:
        case BT_XSTACK_PHONEBOOK_EVENT_CATEGORY_COMPLETED:
        case BT_XSTACK_PHONEBOOK_EVENT_COMPLETED:
        case BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED:
        case BT_XSTACK_PHONEBOOK_EVENT_ERROR:
            return true;
        default:
            return false;
        }
    case BT_XSTACK_EVENT_DOMAIN_LE:
        return event->data.le.type != BT_XSTACK_LE_EVENT_SCAN_RESULT;
    case BT_XSTACK_EVENT_DOMAIN_GATT:
        return (event->data.gatt.type == BT_XSTACK_GATT_EVENT_CLIENT_COMPLETE) ||
            (event->data.gatt.type == BT_XSTACK_GATT_EVENT_SERVER_COMPLETE) ||
            (event->data.gatt.type == BT_XSTACK_GATT_EVENT_SERVER_WRITE) ||
            (event->data.gatt.type == BT_XSTACK_GATT_EVENT_SUBSCRIPTION_CHANGED) ||
            (event->data.gatt.type == BT_XSTACK_GATT_EVENT_INDICATION_COMPLETE);
    default:
        return false;
    }
}

int btstack_core_post_event(const btstack_core_event_envelope_t *event)
{
    bool pbap_record = false;
    bool control_event;
    int result;

    if ((event == NULL) || !bt_xstack_api_initialized) {
        return -1;
    }
    control_event = bt_xstack_api_is_control_event(event);
    if ((event->domain == BT_XSTACK_EVENT_DOMAIN_PHONEBOOK) &&
        ((event->data.phonebook.id == BT_XSTACK_PHONEBOOK_EVENT_CONTACT) ||
         (event->data.phonebook.id == BT_XSTACK_PHONEBOOK_EVENT_CALL))) {
        pbap_record = true;
    } else if (((event->domain == BT_XSTACK_EVENT_DOMAIN_SERVICE) &&
        (event->data.service.id ==
            BT_XSTACK_SERVICE_EVENT_SCAN_RESULT)) ||
        ((event->domain == BT_XSTACK_EVENT_DOMAIN_LE) &&
         (event->data.le.type == BT_XSTACK_LE_EVENT_SCAN_RESULT))) {
        btstack_platform_enter_critical();
        if (btstack_ring_push(
                &bt_xstack_best_effort_event_queue, event)) {
            result = 0;
        } else {
            bt_xstack_best_effort_event_drop_count++;
            result = -1;
        }
        btstack_platform_exit_critical();
        if (result == 0) {
            (void)btstack_platform_notify(
                BTSTACK_WORKER_EVENT, BTSTACK_WORKER_SIGNAL_WORK);
        }
        return result;
    }

    btstack_platform_enter_critical();
    if (!control_event &&
        (btstack_ring_spaces(&bt_xstack_critical_event_queue) <=
            BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE)) {
        if (pbap_record) {
            result = BT_XSTACK_API_EVENT_BACKPRESSURE;
        } else {
            bt_xstack_critical_event_drop_count++;
            result = -1;
        }
    } else if (btstack_ring_push(
            &bt_xstack_critical_event_queue, event)) {
        result = 0;
    } else if (pbap_record) {
        result = BT_XSTACK_API_EVENT_BACKPRESSURE;
    } else {
        bt_xstack_critical_event_drop_count++;
        result = -1;
    }
    btstack_platform_exit_critical();

    if (result == 0) {
        (void)btstack_platform_notify(
            BTSTACK_WORKER_EVENT, BTSTACK_WORKER_SIGNAL_WORK);
    }
    return result;
}

void btstack_frontend_deliver_event(const btstack_core_event_envelope_t *event)
{
    bt_xstack_config_t config;
    if (event == NULL) return;
    btstack_platform_enter_critical();
    memcpy(&config, &bt_xstack_api_config, sizeof(config));
    btstack_platform_exit_critical();
    switch (event->domain) {
    case BT_XSTACK_EVENT_DOMAIN_SERVICE:
        if (config.service_callback != NULL) config.service_callback(&event->data.service, config.context);
        break;
    case BT_XSTACK_EVENT_DOMAIN_CONNECTION:
        if (config.connection_callback != NULL) config.connection_callback(&event->data.connection, config.context);
        break;
    case BT_XSTACK_EVENT_DOMAIN_PHONE:
        if (config.phone_callback != NULL) config.phone_callback(&event->data.phone, config.context);
        if ((event->data.phone.id == BT_XSTACK_PHONE_EVENT_LOST) &&
            btstack_profile_manager_hfp_event_delivered(
                (uint8_t)BTSTACK_HFP_HF_EVENT_CONNECTION_LOST,
                event->data.phone.address.bytes)) {
            btstack_core_request_t request;
            memset(&request, 0, sizeof(request));
            request.id = BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED;
            memcpy(&request.address, &event->data.phone.address,
                sizeof(request.address));
            request.has_address = 1u;
            bt_xstack_api_submit_event_task_request(&request, 1u);
        }
        break;
    case BT_XSTACK_EVENT_DOMAIN_AUDIO:
        if (config.audio_event_callback != NULL) config.audio_event_callback(&event->data.audio, config.context);
        break;
    case BT_XSTACK_EVENT_DOMAIN_PHONEBOOK:
        if (config.phonebook_callback != NULL) {
            config.phonebook_callback(
                &event->data.phonebook, config.context);
        }
        break;
    case BT_XSTACK_EVENT_DOMAIN_LE:
        if (config.le_callback != NULL) config.le_callback(&event->data.le, config.context);
        break;
    case BT_XSTACK_EVENT_DOMAIN_GATT:
        if (config.gatt_callback != NULL) config.gatt_callback(&event->data.gatt, config.context);
        break;
    default:
        break;
    }
}

void btstack_frontend_deliver_cover_art(const bt_xstack_cover_art_event_t *event)
{
    bt_xstack_cover_art_callback_t callback;
    void *context;

    if (event == NULL) return;
    btstack_platform_enter_critical();
    callback = bt_xstack_api_cover_art_callback;
    context = bt_xstack_api_cover_art_context;
    btstack_platform_exit_critical();
    if (callback != NULL) callback(event, context);
}

void btstack_core_media_audio_deliver(const uint8_t *data, uint32_t length)
{
    bt_xstack_media_audio_callback_t callback;
    void *context;
    if ((data == NULL) || (length == 0u)) return;
    btstack_platform_enter_critical();
    callback = bt_xstack_api_config.media_audio_callback;
    context = bt_xstack_api_config.context;
    btstack_platform_exit_critical();
    if (callback != NULL) callback(data, length, context);
}

void btstack_core_call_audio_deliver(const uint8_t *data, uint32_t length)
{
    bt_xstack_call_audio_callback_t callback;
    void *context;
    if ((data == NULL) || (length == 0u)) return;
    btstack_platform_enter_critical();
    callback = bt_xstack_api_config.call_audio_callback;
    context = bt_xstack_api_config.context;
    btstack_platform_exit_critical();
    if (callback != NULL) callback(data, length, context);
}
