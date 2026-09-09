#ifndef __drv_l1_AUD_COMPRESSOR_H__
#define __drv_l1_AUD_COMPRESSOR_H__

#define THRESHOLD_FIX ((int)(32767 * 0.98 + 0.5))
//#define ATTACK_FIX  ((int)(0x7FFF*0.9*0.25 + 0.5))
//#define RELEASE_FIX ((int)(0x7FFF*0.001*0.25 + 0.5))
#define POST_COMPRESSION_UP_BOUND ((int)(32767 * 0.95 + 0.5))

//#define THRESHOLD_FIX ((int)(16384 * 0.98 + 0.5))
#define ATTACK_FIX  ((int)(0x7FFF*0.5*0.25 + 0.5))
#define RELEASE_FIX ((int)(0x7FFF*0.051*0.25 + 0.5))
//#define POST_COMPRESSION_UP_BOUND ((int)(16384 * 0.95 + 0.5))

typedef enum
{
    MIC_DATA_NO_DLY_IN,
    MIC_DATA_IN,
	AGC_MIC_IN,
    PDM_R_L_IN,
    PDM_L_R_IN,
    AGC_PDM_L_IN,
    AGC_PDM_R_IN,
    MIC_PDM_L_IN,
    MIC_PDM_R_IN,
    GAIN_MIC_IN,
    GAIN_PDM_L_IN,
    GAIN_PDM_R_IN,
    DAC_MIC_IN,
    DAC_PDM_L_IN,
    DAC_PDM_R_IN,
    GAIN_ACC_IN,
} AUD_COMPRESSOR_IN;

typedef enum
{
    CDO_DATA_OUT,
	AGC_CDO_15_0_OUT,
    PDM_L_CDO_15_0_OUT,
    PDM_R_CDO_15_0_OUT,
    NR_L_CDO_15_0_OUT,
    NR_R_CDO_15_0_OUT,
    MIC_15_0_CDO_15_0_OUT,
    MIC_31_16_CDO_15_0_OUT,
    GAIN_CDO_15_0_OUT,
    DAC_CDO_15_0_OUT,
    GAIN_ACC_OUT,
    CDO_DATA_OUT_2 = 0x10,
    AGC_CDO_31_16_OUT,
    PDM_L_CDO_31_16_OUT,
    PDM_R_CDO_31_16_OUT,
    NR_L_CDO_31_16_OUT,
    NR_R_CDO_31_16_OUT,
    MIC_15_0_CDO_31_16_OUT,
    MIC_31_16_CDO_31_16_OUT,
    GAIN_CDO_31_16_OUT,
    DAC_CDO_31_16_OUT,
} AUD_COMPRESSOR_OUT;

extern void drv_l1_audio_compressor_attack_release(INT16U attack, INT16U release);
extern void drv_l1_audio_compressor_enable(INT8U enable);
extern void drv_l1_audio_compressor_threshold(INT16U threshold);
extern void drv_l1_audio_compressor_up_bound(INT16U upbound);
extern void drv_l1_audio_compressor_config(INT8U mono, INT8U LR_ch, INT8U I2S_32bLR, INT8U sign, INT8U src_sel);
extern void drv_l1_audio_compressor_set_gain_dB(INT8U GDB2D_idx);
extern void drv_l1_adc_audio_mux_input_select(INT8U com_in);
extern void drv_l1_adc_audio_mux_output_select(INT16U com_out);
extern INT8U drv_l1_audio_compressor_get_enable_status(void);
extern void drv_l1_audio_compressor_init(INT8U enable);
#endif  //__drv_l1_AUD_COMPRESSOR_H__
