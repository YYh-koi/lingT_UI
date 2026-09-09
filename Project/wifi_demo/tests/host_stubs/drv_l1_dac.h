#ifndef BT_AUDIO_HOST_DRV_L1_DAC_H
#define BT_AUDIO_HOST_DRV_L1_DAC_H

#include "application.h"

void drv_l1_dac_timer_stop(void);
void drv_l1_dac_dbf_free(void);
INT8U drv_l1_dac_dbf_dma_channel_get(void);
void drv_l1_dac_mono_set(void);
void drv_l1_dac_stereo_set(void);
void drv_l1_dac_sample_rate_set(INT32U sample_rate);
int drv_l1_dac_cha_dbf_put_callback(
    INT16S *buffer,
    uint32_t samples,
    void (*callback)(INT8U, INT8U));
int drv_l1_dac_cha_dbf_set(INT16S *buffer, uint32_t samples);

#endif
