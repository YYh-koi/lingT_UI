#ifndef BT_XSTACK_TEST_QUEUE_H
#define BT_XSTACK_TEST_QUEUE_H

#include "FreeRTOS.h"

typedef StaticQueue_t *QueueHandle_t;

QueueHandle_t xQueueCreateStatic(
    UBaseType_t length,
    UBaseType_t item_size,
    uint8_t *storage,
    StaticQueue_t *control);

BaseType_t test_xQueueSend(
    QueueHandle_t queue,
    const void *item,
    TickType_t ticks_to_wait);
#define xQueueSend(queue, item, ticks_to_wait) \
    test_xQueueSend((queue), (item), (ticks_to_wait))

BaseType_t xQueueReceive(
    QueueHandle_t queue,
    void *item,
    TickType_t ticks_to_wait);
UBaseType_t uxQueueSpacesAvailable(QueueHandle_t queue);

#endif
