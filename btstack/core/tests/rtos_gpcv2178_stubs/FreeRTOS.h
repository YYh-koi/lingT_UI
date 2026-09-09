#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H

#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;
typedef void *TaskHandle_t;
typedef struct { uintptr_t opaque; } StaticTask_t;

#define pdFAIL 0
#define pdPASS 1
#define pdFALSE 0
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define configMAX_PRIORITIES 8u
#define tskIDLE_PRIORITY 0u
#define pdMS_TO_TICKS(value) ((TickType_t)((value) * 2u))

#endif
