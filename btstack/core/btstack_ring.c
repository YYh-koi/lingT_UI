#include "btstack_ring.h"

#include <string.h>

void btstack_ring_init(
    btstack_ring_t *ring,
    void *storage,
    uint16_t item_size,
    uint16_t capacity)
{
    ring->storage = (uint8_t *)storage;
    ring->item_size = item_size;
    ring->capacity = capacity;
    ring->read_index = 0u;
    ring->write_index = 0u;
    ring->count = 0u;
}

bool btstack_ring_push(btstack_ring_t *ring, const void *item)
{
    if (ring->count == ring->capacity) {
        return false;
    }
    memcpy(
        ring->storage + ((uint32_t)ring->write_index * ring->item_size),
        item,
        ring->item_size);
    ring->write_index++;
    if (ring->write_index == ring->capacity) {
        ring->write_index = 0u;
    }
    ring->count++;
    return true;
}

bool btstack_ring_pop(btstack_ring_t *ring, void *item)
{
    if (ring->count == 0u) {
        return false;
    }
    memcpy(
        item,
        ring->storage + ((uint32_t)ring->read_index * ring->item_size),
        ring->item_size);
    ring->read_index++;
    if (ring->read_index == ring->capacity) {
        ring->read_index = 0u;
    }
    ring->count--;
    return true;
}

uint16_t btstack_ring_count(const btstack_ring_t *ring)
{
    return ring->count;
}

uint16_t btstack_ring_spaces(const btstack_ring_t *ring)
{
    return (uint16_t)(ring->capacity - ring->count);
}
