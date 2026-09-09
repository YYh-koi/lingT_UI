#ifndef BT_AUDIO_HOST_TASK_H
#define BT_AUDIO_HOST_TASK_H

#include "FreeRTOS.h"

uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, uint32_t wait_ticks);
void vTaskNotifyGiveFromISR(
    TaskHandle_t task,
    BaseType_t *higher_priority_task_woken);
void xTaskNotifyGive(TaskHandle_t task);
TaskHandle_t xTaskCreateStatic(
    void (*entry)(void *),
    const char *name,
    uint32_t stack_words,
    void *context,
    uint32_t priority,
    StackType_t *stack,
    StaticTask_t *state);

#endif
