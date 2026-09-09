#ifndef BTSTACK_A2DP_MEDIA_H
#define BTSTACK_A2DP_MEDIA_H

#include <stdint.h>

#include "adapter/btstack_types.h"
#include "events/btstack_event.h"

#ifdef __cplusplus
extern "C" {
#endif

void btstack_a2dp_media_init(
    btstack_audio_sample_callback_t callback);
int btstack_a2dp_media_configure(
    uint32_t sample_rate,
    uint8_t channel_count);
void btstack_a2dp_media_reset(void);
void btstack_a2dp_media_process(
    const uint8_t *packet,
    uint16_t size);

#ifdef __cplusplus
}
#endif

#endif
