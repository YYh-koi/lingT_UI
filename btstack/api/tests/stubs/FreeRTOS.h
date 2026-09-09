#ifndef BT_XSTACK_TEST_FREERTOS_H
#define BT_XSTACK_TEST_FREERTOS_H

#include <stddef.h>
#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;

typedef struct {
    UBaseType_t length;
    UBaseType_t item_size;
    UBaseType_t head;
    UBaseType_t tail;
    UBaseType_t count;
    uint8_t *storage;
} StaticQueue_t;
typedef struct { uintptr_t opaque; } StaticTask_t;

#define pdFAIL 0
#define pdPASS 1
#define pdTRUE 1
#define portMAX_DELAY ((TickType_t)~(TickType_t)0u)
#define configMAX_PRIORITIES 16u
#define tskIDLE_PRIORITY 0u

#endif
