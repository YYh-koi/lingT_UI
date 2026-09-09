#ifndef BT_AUDIO_TASK_H
#define BT_AUDIO_TASK_H

#include <stdint.h>

/* Application-owned DAC diagnostic switch; keep BTstack internals private. */
#ifndef BT_AUDIO_DIAG_BYPASS_DAC_DMA
#define BT_AUDIO_DIAG_BYPASS_DAC_DMA 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BT_AUDIO_SOURCE_IDLE = 0,
    BT_AUDIO_SOURCE_A2DP,
    BT_AUDIO_SOURCE_SCO
} bt_audio_source_t;

typedef struct {
    bt_audio_source_t source;
    uint32_t sco_bytes;
    uint32_t sco_max_bytes;
    uint32_t sco_overflows;
    uint32_t sco_dropped_oldest_bytes;
    uint32_t source_mismatch_drops;
    uint32_t dma_underflows;
    uint32_t silence_bytes;
    uint32_t odd_tail_observations;
} bt_audio_task_diagnostics_t;

int bt_audio_task_start(void);
void bt_audio_task_stop(void);
int bt_audio_task_configure(
    uint32_t sample_rate,
    uint8_t channel_count,
    uint8_t bits_per_sample);
void bt_audio_task_write_pcm(
    const uint8_t *data,
    uint32_t len);
void bt_audio_task_write_media_pcm(
    const uint8_t *data,
    uint32_t len);
void bt_audio_task_write_call_pcm(
    const uint8_t *data,
    uint32_t len);
int bt_audio_task_get_diagnostics(
    bt_audio_task_diagnostics_t *diagnostics);

#ifdef __cplusplus
}
#endif

#endif
