#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../../ports/gpcv2178/btstack_gpcv2178_config.h"
#include "adapter/btstack_types.h"
#include "btstack_rtos_gpcv2178.h"
#include "FreeRTOS.h"
#include "task.h"

static const btstack_worker_ops_t *ops;
static int critical_depth;
static unsigned int static_create_count;
static unsigned int dynamic_create_count;
static unsigned int delete_count;
static unsigned int notify_count;
static uint32_t last_notify_signals;
static BaseType_t next_notify_result = pdPASS;
static int arrange_stop_during_notify;
static int stop_arranged;
static unsigned int delete_count_at_notify;
static uint32_t notification_value;
static int notification_pending;
static uint32_t wait_clear_mask;
static TickType_t wait_ticks;
static TickType_t tick_count;
static unsigned int wait_call_count;
static TickType_t wait_tick_history[8];
static uint32_t wait_script_signals[4];
static TickType_t wait_script_elapsed[4];
static unsigned int wait_script_count;
static unsigned int wait_script_index;
static int reenter_create;
static int reenter_start_result;
static int reenter_notify_result;
static int reenter_stop_result;
static uintptr_t next_handle = 1u;
static const char *last_static_name;
static uint32_t last_static_stack_words;
static UBaseType_t last_static_priority;

#define EXPECTED_AT_STACK_WORDS 2048u

void test_enter_critical(void) { assert(critical_depth == 0); critical_depth++; }
void test_exit_critical(void) { assert(critical_depth == 1); critical_depth--; }

static void noop_entry(void *argument) { (void)argument; }

static void run_reentrant_probe(btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    if (!reenter_create) return;
    reenter_create = 0;
    reenter_start_result = ops->start(
        ops->context, id, noop_entry, NULL, config);
    reenter_notify_result = ops->notify(
        ops->context, id, BTSTACK_WORKER_SIGNAL_WORK);
    reenter_stop_result = ops->stop(ops->context, id);
}

TaskHandle_t xTaskCreateStatic(void (*entry)(void *), const char *name,
    uint32_t stack_words, void *argument, UBaseType_t priority,
    StackType_t *stack, StaticTask_t *control)
{
    btstack_worker_config_t config;
    (void)entry; (void)name; (void)argument; (void)stack; (void)control;
    assert(critical_depth == 0);
    static_create_count++;
    last_static_name = name;
    last_static_stack_words = stack_words;
    last_static_priority = priority;
    config.priority = priority;
    config.stack_words = (uint16_t)stack_words;
    run_reentrant_probe(
        (strcmp(name, "btstack_at") == 0) ?
            BTSTACK_WORKER_AT : BTSTACK_WORKER_API,
        &config);
    return (TaskHandle_t)next_handle++;
}

BaseType_t xTaskCreate(void (*entry)(void *), const char *name,
    uint32_t stack_words, void *argument, UBaseType_t priority,
    TaskHandle_t *handle)
{
    btstack_worker_config_t config;
    (void)entry; (void)name; (void)argument;
    assert(critical_depth == 0);
    dynamic_create_count++;
    config.priority = priority;
    config.stack_words = (uint16_t)stack_words;
    run_reentrant_probe(BTSTACK_WORKER_STACK, &config);
    *handle = (TaskHandle_t)next_handle++;
    return pdPASS;
}

BaseType_t xTaskNotify(TaskHandle_t handle, uint32_t signals,
    eNotifyAction action)
{
    assert(critical_depth == 1);
    assert(handle != NULL);
    assert(action == eSetBits);
    notify_count++;
    last_notify_signals = signals;
    if (arrange_stop_during_notify) {
        stop_arranged = 1;
        delete_count_at_notify = delete_count;
    }
    if (next_notify_result == pdPASS) {
        notification_value |= signals;
        notification_pending = 1;
    }
    return next_notify_result;
}

BaseType_t xTaskNotifyWait(uint32_t clear_on_entry,
    uint32_t clear_on_exit, uint32_t *signals, TickType_t ticks)
{
    BaseType_t result;

    assert(critical_depth == 0);
    assert(clear_on_entry == 0u);
    wait_clear_mask = clear_on_exit;
    wait_ticks = ticks;
    assert(wait_call_count <
        (sizeof(wait_tick_history) / sizeof(wait_tick_history[0])));
    wait_tick_history[wait_call_count++] = ticks;

    if (!notification_pending) {
        if (wait_script_index < wait_script_count) {
            TickType_t elapsed = wait_script_elapsed[wait_script_index];
            assert(elapsed <= ticks);
            tick_count += elapsed;
            notification_value |= wait_script_signals[wait_script_index];
            notification_pending = 1;
            wait_script_index++;
        } else {
            tick_count += ticks;
        }
    }

    *signals = notification_value;
    result = notification_pending ? pdTRUE : pdFALSE;
    if (notification_pending) {
        notification_value &= ~clear_on_exit;
    }
    notification_pending = 0;
    return result;
}

TickType_t xTaskGetTickCount(void)
{
    return tick_count;
}

void vTaskDelete(TaskHandle_t handle)
{
    assert(critical_depth == 0);
    assert(handle != NULL);
    delete_count++;
}

static btstack_worker_config_t config(uint32_t priority,
    uint16_t stack_words)
{
    btstack_worker_config_t value;
    value.priority = priority;
    value.stack_words = stack_words;
    return value;
}

static void script_notification(uint32_t signals, TickType_t elapsed)
{
    assert(wait_script_count <
        (sizeof(wait_script_signals) / sizeof(wait_script_signals[0])));
    wait_script_signals[wait_script_count] = signals;
    wait_script_elapsed[wait_script_count] = elapsed;
    wait_script_count++;
}

static void reset_wait_script(void)
{
    wait_script_count = 0u;
    wait_script_index = 0u;
}

static void test_validation(void)
{
    btstack_worker_config_t value;
    value = config(0u, 0u);
#if defined(BTSTACK_BUILD_VARIANT_API)
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, &value) == 0);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_EVENT, &value) == 0);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_AT, &value) ==
        BTSTACK_STATUS_NOT_READY);
#else
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, &value) ==
        BTSTACK_STATUS_NOT_READY);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_EVENT, &value) ==
        BTSTACK_STATUS_NOT_READY);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_AT, &value) == 0);
#endif
#if defined(BTSTACK_BUILD_VARIANT_API)
    value = config(configMAX_PRIORITIES - 1u,
        BT_XSTACK_API_TASK_STACK_WORDS);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, &value) == 0);
    value.priority = configMAX_PRIORITIES;
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, &value) != 0);
    value = config(1u, BT_XSTACK_API_TASK_STACK_WORDS - 1u);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, &value) != 0);
    value = config(1u, BT_XSTACK_EVENT_TASK_STACK_WORDS);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_EVENT, &value) == 0);
    value.stack_words--;
    assert(ops->validate_config(NULL, BTSTACK_WORKER_EVENT, &value) != 0);
#else
    value = config(1u, EXPECTED_AT_STACK_WORDS);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_AT, &value) == 0);
    value.stack_words--;
    assert(ops->validate_config(NULL, BTSTACK_WORKER_AT, &value) == 0);
    value.stack_words = EXPECTED_AT_STACK_WORDS + 1u;
    assert(ops->validate_config(NULL, BTSTACK_WORKER_AT, &value) != 0);
#endif
    value = config(BTSTACK_GPCV2178_TASK_PRIORITY,
        BTSTACK_GPCV2178_TASK_STACK_SIZE);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_STACK, &value) == 0);
    value.priority--;
    assert(ops->validate_config(NULL, BTSTACK_WORKER_STACK, &value) != 0);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_COUNT, &value) != 0);
    assert(ops->validate_config(NULL, (btstack_worker_id_t)-1, &value) != 0);
    assert(ops->validate_config(NULL, BTSTACK_WORKER_API, NULL) != 0);
}

static void assert_unavailable_worker(btstack_worker_id_t id)
{
    unsigned int waits_before = wait_call_count;
    assert(ops->notify(NULL, id, BTSTACK_WORKER_SIGNAL_WORK) ==
        BTSTACK_STATUS_NOT_READY);
    assert(ops->wait(NULL, id, BTSTACK_WORKER_SIGNAL_WORK, 0u) == 0u);
    assert(wait_call_count == waits_before);
    assert(ops->stop(NULL, id) == BTSTACK_STATUS_NOT_READY);
}

static void test_unavailable_workers_are_rejected(void)
{
#if defined(BTSTACK_BUILD_VARIANT_API)
    assert_unavailable_worker(BTSTACK_WORKER_AT);
#else
    assert_unavailable_worker(BTSTACK_WORKER_API);
    assert_unavailable_worker(BTSTACK_WORKER_EVENT);
#endif
}

static void test_static_dynamic_and_signals(void)
{
#if defined(BTSTACK_BUILD_VARIANT_API)
    btstack_worker_config_t api = config(0u,
        BT_XSTACK_API_TASK_STACK_WORDS);
    btstack_worker_config_t event = config(1u,
        BT_XSTACK_EVENT_TASK_STACK_WORDS);
#else
    btstack_worker_config_t at = config(0u, 0u);
#endif
    btstack_worker_config_t stack = config(
        BTSTACK_GPCV2178_TASK_PRIORITY, BTSTACK_GPCV2178_TASK_STACK_SIZE);
    btstack_worker_id_t signal_worker;
    unsigned int waits_before;
    TickType_t ticks_before;

#if defined(BTSTACK_BUILD_VARIANT_API)
    assert(ops->start(NULL, BTSTACK_WORKER_API,
        noop_entry, NULL, &api) == 0);
    assert(static_create_count == 1u);
    assert(ops->start(NULL, BTSTACK_WORKER_API,
        noop_entry, NULL, &api) == 0);
    assert(static_create_count == 1u);
    assert(ops->start(NULL, BTSTACK_WORKER_EVENT,
        noop_entry, NULL, &event) == 0);
    assert(static_create_count == 2u);
    assert(ops->start(NULL, BTSTACK_WORKER_AT,
        noop_entry, NULL, &api) == BTSTACK_STATUS_NOT_READY);
    signal_worker = BTSTACK_WORKER_EVENT;
#else
    assert(ops->start(NULL, BTSTACK_WORKER_API,
        noop_entry, NULL, &at) == BTSTACK_STATUS_NOT_READY);
    assert(ops->start(NULL, BTSTACK_WORKER_EVENT,
        noop_entry, NULL, &at) == BTSTACK_STATUS_NOT_READY);
    assert(ops->start(NULL, BTSTACK_WORKER_AT,
        noop_entry, NULL, &at) == 0);
    assert(static_create_count == 1u);
    assert(strcmp(last_static_name, "btstack_at") == 0);
    assert(last_static_stack_words == EXPECTED_AT_STACK_WORDS);
    assert(last_static_priority == tskIDLE_PRIORITY + 2u);
    assert(ops->start(NULL, BTSTACK_WORKER_AT,
        noop_entry, NULL, &at) == 0);
    assert(static_create_count == 1u);
    signal_worker = BTSTACK_WORKER_AT;
#endif
    assert(ops->start(NULL, BTSTACK_WORKER_STACK,
        noop_entry, NULL, &stack) == 0);
    assert(dynamic_create_count == 1u);

    assert(ops->notify(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK |
        BTSTACK_WORKER_SIGNAL_COMMAND_SLOT |
        BTSTACK_WORKER_SIGNAL_REQUEST_SPACE |
        BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE) == 0);
    assert(notify_count == 1u);
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK | BTSTACK_WORKER_SIGNAL_REQUEST_SPACE,
        7u) == (BTSTACK_WORKER_SIGNAL_WORK |
            BTSTACK_WORKER_SIGNAL_REQUEST_SPACE));
    waits_before = wait_call_count;
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_COMMAND_SLOT, 0u) ==
        BTSTACK_WORKER_SIGNAL_COMMAND_SLOT);
    assert(wait_call_count == waits_before);
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE, 0u) ==
        BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE);
    assert(wait_call_count == waits_before);
    assert(wait_clear_mask == UINT32_MAX);
    assert(wait_ticks == pdMS_TO_TICKS(7u));
    assert(ops->notify(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK) == 0);
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK, BTSTACK_WORKER_WAIT_FOREVER) ==
        BTSTACK_WORKER_SIGNAL_WORK);
    assert(wait_ticks == portMAX_DELAY);

    assert(ops->notify(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK |
        BTSTACK_WORKER_SIGNAL_COMPLETION) == 0);
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_COMPLETION, 9u) ==
        BTSTACK_WORKER_SIGNAL_COMPLETION);
    waits_before = wait_call_count;
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK, 0u) ==
        BTSTACK_WORKER_SIGNAL_WORK);
    assert(wait_call_count == waits_before);
    assert(wait_clear_mask == UINT32_MAX);
    assert(wait_ticks == pdMS_TO_TICKS(9u));

    reset_wait_script();
    script_notification(BTSTACK_WORKER_SIGNAL_WORK, 4u);
    script_notification(BTSTACK_WORKER_SIGNAL_REQUEST_SPACE, 6u);
    waits_before = wait_call_count;
    ticks_before = tick_count;
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_COMPLETION, 10u) == 0u);
    assert(wait_call_count == waits_before + 3u);
    assert(wait_tick_history[waits_before] == pdMS_TO_TICKS(10u));
    assert(wait_tick_history[waits_before + 1u] ==
        pdMS_TO_TICKS(10u) - 4u);
    assert(wait_tick_history[waits_before + 2u] ==
        pdMS_TO_TICKS(10u) - 10u);
    assert(tick_count - ticks_before == pdMS_TO_TICKS(10u));
    waits_before = wait_call_count;
    assert(ops->wait(NULL, signal_worker,
        BTSTACK_WORKER_SIGNAL_WORK |
        BTSTACK_WORKER_SIGNAL_REQUEST_SPACE, 0u) ==
        (BTSTACK_WORKER_SIGNAL_WORK |
            BTSTACK_WORKER_SIGNAL_REQUEST_SPACE));
    assert(wait_call_count == waits_before);

    next_notify_result = pdFAIL;
    assert(ops->notify(NULL, signal_worker, 0x66u) ==
        BTSTACK_STATUS_NOT_READY);
    next_notify_result = pdPASS;

    arrange_stop_during_notify = 1;
    stop_arranged = 0;
    assert(ops->notify(NULL, signal_worker, 0x77u) == 0);
    assert(stop_arranged);
    assert(delete_count == delete_count_at_notify);
    arrange_stop_during_notify = 0;
    assert(ops->stop(NULL, signal_worker) == 0);
    assert(delete_count == delete_count_at_notify + 1u);

#if defined(BTSTACK_BUILD_VARIANT_API)
    assert(ops->stop(NULL, BTSTACK_WORKER_API) == 0);
    assert(ops->stop(NULL, BTSTACK_WORKER_EVENT) == 0);
#else
    assert(ops->stop(NULL, BTSTACK_WORKER_AT) == 0);
    assert(ops->stop(NULL, BTSTACK_WORKER_AT) == 0);
#endif
    assert(ops->stop(NULL, BTSTACK_WORKER_STACK) == 0);
#if defined(BTSTACK_BUILD_VARIANT_API)
    assert(delete_count == 3u);
#else
    assert(delete_count == 2u);
#endif
    assert(ops->notify(NULL, signal_worker, 1u) != 0);
    assert(ops->notify(NULL, BTSTACK_WORKER_COUNT, 1u) != 0);
    assert(ops->notify(NULL, (btstack_worker_id_t)-1, 1u) != 0);
    assert(ops->wait(NULL, BTSTACK_WORKER_COUNT, 1u, 0u) == 0u);
    assert(ops->wait(NULL, (btstack_worker_id_t)-1, 1u, 0u) == 0u);
    assert(ops->stop(NULL, BTSTACK_WORKER_COUNT) != 0);
    assert(ops->stop(NULL, (btstack_worker_id_t)-1) != 0);
}

static void test_reentrant_create_is_not_duplicated(void)
{
#if defined(BTSTACK_BUILD_VARIANT_API)
    btstack_worker_config_t api = config(0u,
        BT_XSTACK_API_TASK_STACK_WORDS);
    btstack_worker_id_t id = BTSTACK_WORKER_API;
    btstack_worker_config_t selected = api;
#else
    btstack_worker_id_t id = BTSTACK_WORKER_AT;
    btstack_worker_config_t selected = config(0u, 0u);
#endif
    unsigned int before = static_create_count;

    reenter_start_result = 0;
    reenter_notify_result = 0;
    reenter_stop_result = 0;
    reenter_create = 1;
    assert(ops->start(NULL, id,
        noop_entry, NULL, &selected) == 0);
    assert(static_create_count == before + 1u);
    assert(reenter_start_result == BTSTACK_STATUS_NOT_READY);
    assert(reenter_notify_result == BTSTACK_STATUS_NOT_READY);
    assert(reenter_stop_result == BTSTACK_STATUS_NOT_READY);
    assert(ops->notify(NULL, id, 0x80u) == 0);
    assert(ops->stop(NULL, id) == 0);
}

int main(void)
{
    ops = btstack_rtos_gpcv2178_worker_ops();
    assert(ops != NULL);
    test_validation();
    test_unavailable_workers_are_rejected();
    test_static_dynamic_and_signals();
    test_reentrant_create_is_not_duplicated();
    assert(critical_depth == 0);
    return 0;
}
