#ifndef BT_XSTACK_TEST_TASK_H
#define BT_XSTACK_TEST_TASK_H

#include "FreeRTOS.h"

typedef void *TaskHandle_t;

void test_task_enter_critical(void);
void test_task_exit_critical(void);
#define taskENTER_CRITICAL() test_task_enter_critical()
#define taskEXIT_CRITICAL() test_task_exit_critical()
TaskHandle_t test_xTaskCreateStatic(
    void (*task)(void *),
    const char *name,
    uint16_t stack_words,
    void *context,
    UBaseType_t priority,
    StackType_t *stack,
    StaticTask_t *control);
#define xTaskCreateStatic(task, name, stack_words, context, priority, stack, control) \
    test_xTaskCreateStatic((task), (name), (stack_words), (context), \
        (priority), (stack), (control))

uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t ticks_to_wait);
BaseType_t xTaskNotifyGive(TaskHandle_t task);

#endif
