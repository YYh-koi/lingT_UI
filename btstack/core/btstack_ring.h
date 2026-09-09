#ifndef BTSTACK_RING_H
#define BTSTACK_RING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *storage;
    uint16_t item_size;
    uint16_t capacity;
    uint16_t read_index;
    uint16_t write_index;
    uint16_t count;
} btstack_ring_t;

void btstack_ring_init(
    btstack_ring_t *ring,
    void *storage,
    uint16_t item_size,
    uint16_t capacity);
bool btstack_ring_push(btstack_ring_t *ring, const void *item);
bool btstack_ring_pop(btstack_ring_t *ring, void *item);
uint16_t btstack_ring_count(const btstack_ring_t *ring);
uint16_t btstack_ring_spaces(const btstack_ring_t *ring);

#endif
