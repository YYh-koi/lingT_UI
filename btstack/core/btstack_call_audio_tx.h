#ifndef BTSTACK_CALL_AUDIO_TX_H
#define BTSTACK_CALL_AUDIO_TX_H

#include <stdbool.h>
#include <stdint.h>

#include "api/bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void btstack_call_audio_tx_init(void);
void btstack_call_audio_tx_session_changed(
    bool connected,
    uint16_t frame_length);
bt_xstack_result_t btstack_call_audio_tx_write(
    const uint8_t *data,
    uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
