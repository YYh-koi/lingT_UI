#include "btstack_rtos_gpcv2178.h"

#include "btstack_config.h"
#include "adapter/btstack_types.h"
#include "FreeRTOS.h"
#include "task.h"

#define BTSTACK_RTOS_GPCV2178_API_PRIORITY_DEFAULT \
    (tskIDLE_PRIORITY + 2u)
#define BTSTACK_RTOS_GPCV2178_AT_STACK_WORDS 2048u

#if defined(BTSTACK_BUILD_VARIANT_API) && \
    (BTSTACK_BUILD_VARIANT_API == 1) && \
    !defined(BTSTACK_BUILD_VARIANT_AT)
#define BTSTACK_GPCV2178_SELECTED_API 1
#elif defined(BTSTACK_BUILD_VARIANT_AT) && \
    (BTSTACK_BUILD_VARIANT_AT == 1) && \
    !defined(BTSTACK_BUILD_VARIANT_API)
#define BTSTACK_GPCV2178_SELECTED_AT 1
#else
#error "Define exactly one BTstack build variant with value 1"
#endif

typedef struct {
    StaticTask_t *control;
    StackType_t *stack;
    TaskHandle_t handle;
    const char *name;
    uint16_t stack_words;
    uint8_t state;
    uint32_t pending_signals;
} btstack_rtos_gpcv2178_worker_t;

enum {
    BTSTACK_RTOS_WORKER_STOPPED = 0,
    BTSTACK_RTOS_WORKER_STARTING,
    BTSTACK_RTOS_WORKER_RUNNING,
    BTSTACK_RTOS_WORKER_STOPPING
};

#if defined(BTSTACK_GPCV2178_SELECTED_API)
static StaticTask_t btstack_api_control;
static StaticTask_t btstack_event_control;
static StackType_t btstack_api_stack[BT_XSTACK_API_TASK_STACK_WORDS];
static StackType_t btstack_event_stack[BT_XSTACK_EVENT_TASK_STACK_WORDS];
#elif defined(BTSTACK_GPCV2178_SELECTED_AT)
static StaticTask_t btstack_at_control;
static StackType_t btstack_at_stack[BTSTACK_RTOS_GPCV2178_AT_STACK_WORDS];
#endif
static btstack_rtos_gpcv2178_worker_t workers[BTSTACK_WORKER_COUNT] = {
    [BTSTACK_WORKER_STACK] = {
        .name = BTSTACK_GPCV2178_TASK_NAME,
        .stack_words = BTSTACK_GPCV2178_TASK_STACK_SIZE
    },
#if defined(BTSTACK_GPCV2178_SELECTED_API)
    [BTSTACK_WORKER_API] = {
        .control = &btstack_api_control,
        .stack = btstack_api_stack,
        .name = "xstack_api",
        .stack_words = BT_XSTACK_API_TASK_STACK_WORDS
    },
    [BTSTACK_WORKER_EVENT] = {
        .control = &btstack_event_control,
        .stack = btstack_event_stack,
        .name = "xstack_evt",
        .stack_words = BT_XSTACK_EVENT_TASK_STACK_WORDS
    },
#elif defined(BTSTACK_GPCV2178_SELECTED_AT)
    [BTSTACK_WORKER_AT] = {
        .control = &btstack_at_control,
        .stack = btstack_at_stack,
        .name = "btstack_at",
        .stack_words = BTSTACK_RTOS_GPCV2178_AT_STACK_WORDS
    }
#endif
};

static int btstack_rtos_gpcv2178_id_valid(btstack_worker_id_t id)
{
    return ((unsigned int)id < (unsigned int)BTSTACK_WORKER_COUNT);
}

static int btstack_rtos_gpcv2178_worker_available(btstack_worker_id_t id)
{
#if defined(BTSTACK_GPCV2178_SELECTED_API)
    return id != BTSTACK_WORKER_AT;
#elif defined(BTSTACK_GPCV2178_SELECTED_AT)
    return (id != BTSTACK_WORKER_API) &&
        (id != BTSTACK_WORKER_EVENT);
#else
    (void)id;
    return 0;
#endif
}

static int btstack_rtos_gpcv2178_validate(void *context,
    btstack_worker_id_t id, const btstack_worker_config_t *config)
{
    (void)context;
    if ((config == NULL) || !btstack_rtos_gpcv2178_id_valid(id)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_rtos_gpcv2178_worker_available(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (id == BTSTACK_WORKER_API) {
        if ((config->stack_words != 0u) &&
            (config->stack_words != BT_XSTACK_API_TASK_STACK_WORDS)) {
            return BTSTACK_STATUS_INVALID_ARGUMENT;
        }
    } else if (id == BTSTACK_WORKER_EVENT) {
        if ((config->stack_words != 0u) &&
            (config->stack_words != BT_XSTACK_EVENT_TASK_STACK_WORDS)) {
            return BTSTACK_STATUS_INVALID_ARGUMENT;
        }
    } else if (id == BTSTACK_WORKER_AT) {
        if (config->stack_words >
            BTSTACK_RTOS_GPCV2178_AT_STACK_WORDS) {
            return BTSTACK_STATUS_INVALID_ARGUMENT;
        }
    } else {
        if (((config->stack_words != 0u) &&
             (config->stack_words != BTSTACK_GPCV2178_TASK_STACK_SIZE)) ||
            ((config->priority != 0u) &&
             (config->priority != BTSTACK_GPCV2178_TASK_PRIORITY))) {
            return BTSTACK_STATUS_INVALID_ARGUMENT;
        }
    }
    if ((config->priority != 0u) &&
        (config->priority >= configMAX_PRIORITIES)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    return BTSTACK_STATUS_OK;
}

static int btstack_rtos_gpcv2178_start(void *context,
    btstack_worker_id_t id, btstack_worker_entry_t entry, void *argument,
    const btstack_worker_config_t *config)
{
    btstack_rtos_gpcv2178_worker_t *worker;
    TaskHandle_t handle = NULL;
    UBaseType_t priority;
    uint16_t stack_words;
    int validate_result;
    int result = BTSTACK_STATUS_NOT_READY;
    (void)context;

    if (entry == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    validate_result = btstack_rtos_gpcv2178_validate(NULL, id, config);
    if (validate_result != BTSTACK_STATUS_OK) return validate_result;
    worker = &workers[id];
    taskENTER_CRITICAL();
    if (worker->state == BTSTACK_RTOS_WORKER_RUNNING) {
        taskEXIT_CRITICAL();
        return BTSTACK_STATUS_OK;
    }
    if (worker->state != BTSTACK_RTOS_WORKER_STOPPED) {
        taskEXIT_CRITICAL();
        return BTSTACK_STATUS_NOT_READY;
    }
    worker->state = BTSTACK_RTOS_WORKER_STARTING;
    worker->pending_signals = 0u;
    taskEXIT_CRITICAL();
    priority = (config->priority != 0u) ?
        (UBaseType_t)config->priority :
        ((id == BTSTACK_WORKER_STACK) ?
            (UBaseType_t)BTSTACK_GPCV2178_TASK_PRIORITY :
            (UBaseType_t)BTSTACK_RTOS_GPCV2178_API_PRIORITY_DEFAULT);
    stack_words = ((id == BTSTACK_WORKER_AT) &&
        (config->stack_words != 0u)) ?
        config->stack_words : worker->stack_words;
    if (id == BTSTACK_WORKER_STACK) {
        if (xTaskCreate(entry, worker->name, worker->stack_words,
                argument, priority, &handle) == pdPASS) {
            result = BTSTACK_STATUS_OK;
        }
    } else {
        handle = xTaskCreateStatic(entry, worker->name,
            stack_words, argument, priority, worker->stack,
            worker->control);
        if (handle != NULL) result = BTSTACK_STATUS_OK;
    }
    taskENTER_CRITICAL();
    if (result == BTSTACK_STATUS_OK) {
        worker->handle = handle;
        worker->state = BTSTACK_RTOS_WORKER_RUNNING;
    } else {
        worker->handle = NULL;
        worker->state = BTSTACK_RTOS_WORKER_STOPPED;
    }
    taskEXIT_CRITICAL();
    return result;
}

static int btstack_rtos_gpcv2178_notify(void *context,
    btstack_worker_id_t id, uint32_t signals)
{
    TaskHandle_t handle;
    BaseType_t notify_result;
    (void)context;
    if (!btstack_rtos_gpcv2178_id_valid(id) ||
        !btstack_rtos_gpcv2178_worker_available(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    /*
     * Keep the port-private task handle alive across xTaskNotify(). The same
     * FreeRTOS critical section serializes stop's RUNNING -> STOPPING claim,
     * so vTaskDelete() cannot overtake an in-flight notification. This lock
     * protects only the RTOS handle lifecycle; it is not a common ring lock.
     */
    taskENTER_CRITICAL();
    handle = (workers[id].state == BTSTACK_RTOS_WORKER_RUNNING) ?
        workers[id].handle : NULL;
    if (handle == NULL) {
        taskEXIT_CRITICAL();
        return BTSTACK_STATUS_NOT_READY;
    }
    notify_result = xTaskNotify(handle, signals, eSetBits);
    taskEXIT_CRITICAL();
    return (notify_result == pdPASS) ? BTSTACK_STATUS_OK :
        BTSTACK_STATUS_NOT_READY;
}

static uint32_t btstack_rtos_gpcv2178_wait(void *context,
    btstack_worker_id_t id, uint32_t mask, uint32_t timeout_ms)
{
    btstack_rtos_gpcv2178_worker_t *worker;
    uint32_t signals;
    uint32_t matched;
    TickType_t timeout_ticks;
    TickType_t start_ticks = 0u;
    TickType_t remaining_ticks;
    int wait_forever;
    (void)context;
    if (!btstack_rtos_gpcv2178_id_valid(id) ||
        !btstack_rtos_gpcv2178_worker_available(id)) return 0u;
    worker = &workers[id];
    wait_forever = (timeout_ms == BTSTACK_WORKER_WAIT_FOREVER);
    timeout_ticks = wait_forever ?
        portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    remaining_ticks = timeout_ticks;
    if (!wait_forever) {
        start_ticks = xTaskGetTickCount();
    }

    for (;;) {
        taskENTER_CRITICAL();
        matched = worker->pending_signals & mask;
        worker->pending_signals &= ~matched;
        taskEXIT_CRITICAL();
        if (matched != 0u) return matched;

        signals = 0u;
        if (xTaskNotifyWait(0u, UINT32_MAX, &signals,
                remaining_ticks) != pdTRUE) {
            return 0u;
        }
        matched = signals & mask;
        taskENTER_CRITICAL();
        worker->pending_signals |= signals & ~mask;
        taskEXIT_CRITICAL();
        if (matched != 0u) return matched;
        if (!wait_forever) {
            TickType_t elapsed = xTaskGetTickCount() - start_ticks;
            if (elapsed >= timeout_ticks) return 0u;
            remaining_ticks = timeout_ticks - elapsed;
        }
    }
}

static int btstack_rtos_gpcv2178_stop(void *context,
    btstack_worker_id_t id)
{
    TaskHandle_t handle;
    (void)context;
    if (!btstack_rtos_gpcv2178_id_valid(id)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_rtos_gpcv2178_worker_available(id)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    taskENTER_CRITICAL();
    if (workers[id].state == BTSTACK_RTOS_WORKER_STOPPED) {
        taskEXIT_CRITICAL();
        return BTSTACK_STATUS_OK;
    }
    if (workers[id].state != BTSTACK_RTOS_WORKER_RUNNING) {
        taskEXIT_CRITICAL();
        return BTSTACK_STATUS_NOT_READY;
    }
    workers[id].state = BTSTACK_RTOS_WORKER_STOPPING;
    handle = workers[id].handle;
    workers[id].handle = NULL;
    taskEXIT_CRITICAL();
    vTaskDelete(handle);
    taskENTER_CRITICAL();
    workers[id].state = BTSTACK_RTOS_WORKER_STOPPED;
    workers[id].pending_signals = 0u;
    taskEXIT_CRITICAL();
    return BTSTACK_STATUS_OK;
}

static const btstack_worker_ops_t worker_ops = {
    .validate_config = btstack_rtos_gpcv2178_validate,
    .start = btstack_rtos_gpcv2178_start,
    .notify = btstack_rtos_gpcv2178_notify,
    .wait = btstack_rtos_gpcv2178_wait,
    .stop = btstack_rtos_gpcv2178_stop
};

const btstack_worker_ops_t *btstack_rtos_gpcv2178_worker_ops(void)
{
    return &worker_ops;
}
