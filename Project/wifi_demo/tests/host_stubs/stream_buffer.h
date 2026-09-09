#ifndef BT_AUDIO_HOST_STREAM_BUFFER_H
#define BT_AUDIO_HOST_STREAM_BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct { uint32_t unused; } StaticStreamBuffer_t;
typedef void *StreamBufferHandle_t;

StreamBufferHandle_t xStreamBufferCreateStatic(
    size_t capacity,
    size_t trigger_level,
    uint8_t *storage,
    StaticStreamBuffer_t *state);
size_t xStreamBufferSend(
    StreamBufferHandle_t stream,
    const void *data,
    size_t len,
    uint32_t wait_ticks);
size_t xStreamBufferReceive(
    StreamBufferHandle_t stream,
    void *data,
    size_t len,
    uint32_t wait_ticks);
size_t xStreamBufferBytesAvailable(StreamBufferHandle_t stream);
int xStreamBufferReset(StreamBufferHandle_t stream);

#endif
