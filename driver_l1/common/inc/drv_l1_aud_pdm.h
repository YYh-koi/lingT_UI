#ifndef __drv_l1_AUD_PDM_H__
#define __drv_l1_AUD_PDM_H__

/*
000:  2-bit left ch. input = {1・b0,PDM_LCH}
      2-bit right ch. input = {1・b0,PDM_RCH}
001:  2-bit left ch. input = {1・b1,PDM_LCH}
      2-bit right ch. input = {1・b1,PDM_RCH}
010:  2-bit left ch. input = {PDM_LCH,1・b0}
      2-bit right ch. input = {PDM_RCH,1・b0}
011:  2-bit left ch. input = {PDM_LCH,1・b1}
      2-bit right ch. input = {PDM_RCH,1・b1}
100:  2-bit left ch. input = {PDM_LCH_1d,PDM_LCH}
      2-bit right ch. input = {PDM_RCH_1d,PDM_RCH}
101:  2-bit left ch. input = {PDM_LCH,PDM_LCH_1d}
      2-bit right ch. input = {PDM_RCH,PDM_RCH_1d}
110:  2-bit left ch. input = {# of 1 in 3 shift register}
      2-bit right ch. input = {# of 1 in 3 shift register}
111:  2-bit left ch. input = {1・b0,PDM_LCH}
      2-bit right ch. input = {1・b0,PDM_RCH}
*/
typedef enum
{
    ZERO_PDM_CH,
    ONE_PDM_CH,
    PDM_CH_ZERO,
    PDM_CH_ONE,
    DLY1D_PDM_CH,
    PDM_CH_DLY1D,
    SHIFT_1IN3_CH,
    ZERO_PDM_CH2,
} PDM_OUT_SELECT;

extern void drv_l1_pdm_enable(INT8U enable);
extern void drv_l1_pdm_AGC_data_LR_select(INT8U sel);
extern void drv_l1_pdm_sample_edge(INT8U edge);
extern void drv_l1_pdm_LRC_data_select(INT8U sel);
#endif		// __drv_l1_AUD_PDM_H__
