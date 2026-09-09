#include <assert.h>
#include <stdint.h>

#include "btstack_ring.h"

int main(void)
{
    uint32_t storage[3];
    btstack_ring_t ring;
    uint32_t value;

    btstack_ring_init(&ring, storage, sizeof(storage[0]), 3u);
    assert(btstack_ring_count(&ring) == 0u);
    assert(btstack_ring_spaces(&ring) == 3u);
    assert(!btstack_ring_pop(&ring, &value));

    value = 10u;
    assert(btstack_ring_push(&ring, &value));
    value = 20u;
    assert(btstack_ring_push(&ring, &value));
    value = 30u;
    assert(btstack_ring_push(&ring, &value));
    assert(btstack_ring_count(&ring) == 3u);
    assert(btstack_ring_spaces(&ring) == 0u);
    value = 40u;
    assert(!btstack_ring_push(&ring, &value));

    assert(btstack_ring_pop(&ring, &value));
    assert(value == 10u);
    value = 40u;
    assert(btstack_ring_push(&ring, &value));
    assert(btstack_ring_count(&ring) == 3u);

    assert(btstack_ring_pop(&ring, &value));
    assert(value == 20u);
    assert(btstack_ring_pop(&ring, &value));
    assert(value == 30u);
    assert(btstack_ring_pop(&ring, &value));
    assert(value == 40u);
    assert(!btstack_ring_pop(&ring, &value));
    assert(btstack_ring_count(&ring) == 0u);
    assert(btstack_ring_spaces(&ring) == 3u);
    return 0;
}
