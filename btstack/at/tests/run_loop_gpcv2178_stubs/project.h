#ifndef BTSTACK_TEST_PROJECT_H
#define BTSTACK_TEST_PROJECT_H

#include <stdint.h>

typedef int BaseType_t;
typedef uint32_t TickType_t;
typedef void *SemaphoreHandle_t;

#define pdFALSE 0
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define portTICK_PERIOD_MS 1u
#define pdMS_TO_TICKS(milliseconds) ((TickType_t)(milliseconds))

void test_task_enter_critical(void);
void test_task_exit_critical(void);
#define taskENTER_CRITICAL() test_task_enter_critical()
#define taskEXIT_CRITICAL() test_task_exit_critical()

SemaphoreHandle_t xSemaphoreCreateBinary(void);
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks);
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t semaphore,
    BaseType_t *higher_priority_task_woken);
TickType_t xTaskGetTickCount(void);
void vTaskDelay(TickType_t ticks);
void test_port_yield_from_isr(BaseType_t higher_priority_task_woken);
#define portYIELD_FROM_ISR(value) test_port_yield_from_isr(value)

#endif
