#ifndef TEST_TASK_H
#define TEST_TASK_H

#include "FreeRTOS.h"

typedef enum { eNoAction = 0, eSetBits = 1 } eNotifyAction;

void test_enter_critical(void);
void test_exit_critical(void);
#define taskENTER_CRITICAL() test_enter_critical()
#define taskEXIT_CRITICAL() test_exit_critical()

TaskHandle_t xTaskCreateStatic(void (*entry)(void *), const char *name,
    uint32_t stack_words, void *argument, UBaseType_t priority,
    StackType_t *stack, StaticTask_t *control);
BaseType_t xTaskCreate(void (*entry)(void *), const char *name,
    uint32_t stack_words, void *argument, UBaseType_t priority,
    TaskHandle_t *handle);
BaseType_t xTaskNotify(TaskHandle_t handle, uint32_t signals,
    eNotifyAction action);
BaseType_t xTaskNotifyWait(uint32_t clear_on_entry,
    uint32_t clear_on_exit, uint32_t *signals, TickType_t ticks);
TickType_t xTaskGetTickCount(void);
void vTaskDelete(TaskHandle_t handle);

#endif
