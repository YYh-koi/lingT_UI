#include "btstack_run_loop_gpcv2178.h"

#include "btstack_debug.h"
#include "project.h"
#include "port/btstack_platform.h"

#define BTSTACK_RUN_LOOP_INGRESS_CAPACITY 16u

static SemaphoreHandle_t btstack_run_loop_sem;
static volatile int btstack_run_loop_exit_requested;
static btstack_context_callback_registration_t
    *btstack_run_loop_ingress[BTSTACK_RUN_LOOP_INGRESS_CAPACITY];
static uint8_t btstack_run_loop_ingress_read;
static uint8_t btstack_run_loop_ingress_write;
static uint8_t btstack_run_loop_ingress_count;
static uint32_t btstack_run_loop_ingress_max_depth;
static uint32_t btstack_run_loop_ingress_full_count;

static uint32_t btstack_run_loop_gpcv2178_get_time_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static void btstack_run_loop_gpcv2178_init(void)
{
    uint8_t index;

    btstack_run_loop_base_init();
    btstack_run_loop_exit_requested = 0;
    btstack_run_loop_ingress_read = 0u;
    btstack_run_loop_ingress_write = 0u;
    btstack_run_loop_ingress_count = 0u;
    btstack_run_loop_ingress_max_depth = 0u;
    btstack_run_loop_ingress_full_count = 0u;
    for (index = 0u; index < BTSTACK_RUN_LOOP_INGRESS_CAPACITY;
         index++) {
        btstack_run_loop_ingress[index] = NULL;
    }
    btstack_run_loop_sem = xSemaphoreCreateBinary();
}

static void btstack_run_loop_gpcv2178_add_data_source(btstack_data_source_t *data_source)
{
    btstack_run_loop_base_add_data_source(data_source);
}

static bool btstack_run_loop_gpcv2178_remove_data_source(btstack_data_source_t *data_source)
{
    return btstack_run_loop_base_remove_data_source(data_source);
}

static void btstack_run_loop_gpcv2178_enable_data_source_callbacks(btstack_data_source_t *data_source, uint16_t callbacks)
{
    btstack_run_loop_base_enable_data_source_callbacks(data_source, callbacks);
}

static void btstack_run_loop_gpcv2178_disable_data_source_callbacks(btstack_data_source_t *data_source, uint16_t callbacks)
{
    btstack_run_loop_base_disable_data_source_callbacks(data_source, callbacks);
}

static void btstack_run_loop_gpcv2178_set_timer(btstack_timer_source_t *timer, uint32_t timeout_in_ms)
{
    timer->timeout = btstack_run_loop_gpcv2178_get_time_ms() + timeout_in_ms;
}

static void btstack_run_loop_gpcv2178_add_timer(btstack_timer_source_t *timer)
{
    btstack_run_loop_base_add_timer(timer);
    btstack_run_loop_gpcv2178_wake();
}

static bool btstack_run_loop_gpcv2178_remove_timer(btstack_timer_source_t *timer)
{
    return btstack_run_loop_base_remove_timer(timer);
}

void btstack_run_loop_gpcv2178_wake(void)
{
    if (btstack_run_loop_sem != NULL) {
        xSemaphoreGive(btstack_run_loop_sem);
    }
}

bool btstack_run_loop_gpcv2178_enqueue_callback(
    btstack_context_callback_registration_t *callback_registration)
{
    uint8_t offset;
    bool queued = false;
    bool duplicate = false;

    if (callback_registration == NULL) {
        return false;
    }

    taskENTER_CRITICAL();
    for (offset = 0u; offset < btstack_run_loop_ingress_count; offset++) {
        uint8_t index = (uint8_t)(
            (btstack_run_loop_ingress_read + offset) %
            BTSTACK_RUN_LOOP_INGRESS_CAPACITY);
        if (btstack_run_loop_ingress[index] == callback_registration) {
            duplicate = true;
            break;
        }
    }
    if (duplicate) {
        queued = true;
    } else if (btstack_run_loop_ingress_count ==
               BTSTACK_RUN_LOOP_INGRESS_CAPACITY) {
        btstack_run_loop_ingress_full_count++;
    } else {
        btstack_run_loop_ingress[btstack_run_loop_ingress_write] =
            callback_registration;
        btstack_run_loop_ingress_write = (uint8_t)(
            (btstack_run_loop_ingress_write + 1u) %
            BTSTACK_RUN_LOOP_INGRESS_CAPACITY);
        btstack_run_loop_ingress_count++;
        if (btstack_run_loop_ingress_count >
            btstack_run_loop_ingress_max_depth) {
            btstack_run_loop_ingress_max_depth =
                btstack_run_loop_ingress_count;
        }
        queued = true;
    }
    taskEXIT_CRITICAL();

    if (queued && !duplicate) {
        btstack_run_loop_gpcv2178_wake();
    }
    return queued;
}

static btstack_context_callback_registration_t *
btstack_run_loop_gpcv2178_dequeue_callback(void)
{
    btstack_context_callback_registration_t *callback_registration = NULL;

    taskENTER_CRITICAL();
    if (btstack_run_loop_ingress_count != 0u) {
        callback_registration =
            btstack_run_loop_ingress[btstack_run_loop_ingress_read];
        btstack_run_loop_ingress[btstack_run_loop_ingress_read] = NULL;
        btstack_run_loop_ingress_read = (uint8_t)(
            (btstack_run_loop_ingress_read + 1u) %
            BTSTACK_RUN_LOOP_INGRESS_CAPACITY);
        btstack_run_loop_ingress_count--;
    }
    taskEXIT_CRITICAL();
#if defined(BTSTACK_BUILD_VARIANT_AT) && \
    (BTSTACK_BUILD_VARIANT_AT == 1)
    if (callback_registration != NULL) {
        /* The dequeue, not an unrelated AT event, is the reliable edge
         * proving that a previously full stack ingress has space. */
        (void)btstack_platform_notify(
            BTSTACK_WORKER_AT,
            BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE);
    }
#endif
    return callback_registration;
}

static void btstack_run_loop_gpcv2178_drain_ingress(void)
{
    btstack_context_callback_registration_t *callback_registration;

    while ((callback_registration =
                btstack_run_loop_gpcv2178_dequeue_callback()) != NULL) {
        if (callback_registration->callback != NULL) {
            callback_registration->callback(
                callback_registration->context);
        }
    }
}

void btstack_run_loop_gpcv2178_get_diagnostics(
    btstack_run_loop_gpcv2178_diagnostics_t *diagnostics)
{
    if (diagnostics == NULL) {
        return;
    }
    taskENTER_CRITICAL();
    diagnostics->current_depth = btstack_run_loop_ingress_count;
    diagnostics->max_depth = btstack_run_loop_ingress_max_depth;
    diagnostics->full_count = btstack_run_loop_ingress_full_count;
    taskEXIT_CRITICAL();
}

static void btstack_run_loop_gpcv2178_execute(void)
{
    while (!btstack_run_loop_exit_requested) {
        uint32_t now_ms;
        int32_t timeout_ms;
        TickType_t wait_ticks;

        btstack_run_loop_gpcv2178_drain_ingress();
        btstack_run_loop_base_execute_callbacks();
        now_ms = btstack_run_loop_gpcv2178_get_time_ms();
        btstack_run_loop_base_process_timers(now_ms);
        btstack_run_loop_base_poll_data_sources();

        timeout_ms = btstack_run_loop_base_get_time_until_timeout(btstack_run_loop_gpcv2178_get_time_ms());
        wait_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS((uint32_t)timeout_ms);

        if (btstack_run_loop_sem != NULL) {
            xSemaphoreTake(btstack_run_loop_sem, wait_ticks);
        } else {
            vTaskDelay(wait_ticks);
        }
    }
}

static void btstack_run_loop_gpcv2178_poll_data_sources_from_irq(void)
{
    btstack_run_loop_gpcv2178_wake_from_irq();
}

static void btstack_run_loop_gpcv2178_execute_on_main_thread(btstack_context_callback_registration_t *callback_registration)
{
    (void)btstack_run_loop_gpcv2178_enqueue_callback(
        callback_registration);
}

static void btstack_run_loop_gpcv2178_trigger_exit(void)
{
    btstack_run_loop_exit_requested = 1;
    btstack_run_loop_gpcv2178_wake();
}

void btstack_run_loop_gpcv2178_wake_from_irq(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (btstack_run_loop_sem != NULL) {
        xSemaphoreGiveFromISR(btstack_run_loop_sem, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static const btstack_run_loop_t btstack_run_loop_gpcv2178 = {
    &btstack_run_loop_gpcv2178_init,
    &btstack_run_loop_gpcv2178_add_data_source,
    &btstack_run_loop_gpcv2178_remove_data_source,
    &btstack_run_loop_gpcv2178_enable_data_source_callbacks,
    &btstack_run_loop_gpcv2178_disable_data_source_callbacks,
    &btstack_run_loop_gpcv2178_set_timer,
    &btstack_run_loop_gpcv2178_add_timer,
    &btstack_run_loop_gpcv2178_remove_timer,
    &btstack_run_loop_gpcv2178_execute,
    &btstack_run_loop_base_dump_timer,
    &btstack_run_loop_gpcv2178_get_time_ms,
    &btstack_run_loop_gpcv2178_poll_data_sources_from_irq,
    &btstack_run_loop_gpcv2178_execute_on_main_thread,
    &btstack_run_loop_gpcv2178_trigger_exit
};

const btstack_run_loop_t *btstack_run_loop_gpcv2178_get_instance(void)
{
    return &btstack_run_loop_gpcv2178;
}
