#ifndef BT_AUDIO_HOST_FREERTOS_H
#define BT_AUDIO_HOST_FREERTOS_H

#include <stdint.h>

typedef int BaseType_t;
typedef uint32_t StackType_t;
typedef struct { uint32_t unused; } StaticTask_t;
typedef void *TaskHandle_t;

#define pdFALSE 0
#define pdTRUE 1
#define portMAX_DELAY UINT32_MAX
#define tskIDLE_PRIORITY 0u
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
#define portYIELD_FROM_ISR(value) ((void)(value))

#endif
