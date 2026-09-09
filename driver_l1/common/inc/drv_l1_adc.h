#ifndef __DRV_L1_ADC_H__
#define __DRV_L1_ADC_H__

#define ADC_LINE_0		1
#define ADC_LINE_1		2
#define ADC_LINE_2		3
#define ADC_LINE_3		4
#define ADC_LINE_4		5
#define ADC_LINE_5		6
#define ADC_LINE_6		7
#define ADC_LINE_7		8
#define ADC_LINE_8		9   // Internal core power 1.1V


#define ADC_AS_TIMER_A	0
#define ADC_AS_TIMER_B	1
#define ADC_AS_TIMER_C	2
#define ADC_AS_TIMER_D	3
#define ADC_AS_TIMER_E	4
#define ADC_AS_TIMER_F	5

//for R_ADC_TEMP_CTR (0xC00D0020)
#define UPPER_SEQ_EN            (1 << 15)  //Upper boundary temp detection squential mode enable
#define LOWER_SEQ_EN            (1 << 14)  //Lower boundary temp detection squential mode enable
#define TEMP_DET_INTR_EN        (1 << 12)  //temp detection interrupt enable
#define TEMP_DET_AVE_SAMPLE     (0xF << 8))//temp detection adc sample number
#define TEMP_DET_THR_SEL        (1 << 7))  //temp detection upper/lower boudary selection
#define TEMP_UB_EN              (1 << 6))  //temp detection upper boudary enable
#define TEMP_LB_EN              (1 << 5))  //temp detection lower boudary enable
#define TEMP_ADC_CH             (0xF << 1) //temp detection adc channel
#define TEMP_BGAP_GND_DET_EN    (1 << 0)   //temp/bandgap/ground detection enable



typedef struct
{
	INT8S *notify;
	INT16U *adc_data;
	INT32U data_len;
	INT32U count;
} ADC_CTRL;

extern void drv_l1_adc_init(INT8U vic_adc, INT8U vic_adcf);
extern void drv_l1_adc_uninit(INT8U vic_adc, INT8U vic_adcf);
extern void drv_l1_adc_vref_enable_set(BOOLEAN status);
extern INT32S drv_l1_adc_conv_time_sel(INT32U value);
extern void drv_l1_adc_fifo_level_set(INT8U level);
extern void drv_l1_adc_fifo_clear(void);
extern void drv_l1_adc_auto_ch_set(INT8U ch);
extern void drv_l1_adc_manual_ch_set(INT8U ch);
extern void drv_l1_adc_auto_int_set(BOOLEAN status);
extern void drv_l1_adc_manual_callback_set(void (*user_isr)(INT16U data));
extern INT32S drv_l1_adc_user_line_in_en(INT8U ch, INT32U enable);
extern INT32S drv_l1_adc_manual_sample_start(void);
extern INT32S drv_l1_adc_auto_sample_start(void);
extern void drv_l1_adc_auto_sample_stop(void);
extern INT32S drv_l1_adc_sample_rate_set(INT8U timer_id, INT32U hz);
extern INT32S drv_l1_adc_timer_stop(INT8U timer_id);

extern void drv_l1_tp_en(BOOLEAN status);
extern void drv_l1_tp_int_en(BOOLEAN status);
extern void drv_l1_tp_callback_set(void (*user_isr)(void));

extern INT32S drv_l1_adc_auto_data_get(INT16U *data, INT32U len, INT8S *notify);
extern INT32S drv_l1_adc_auto_data_dma_get(INT16U *data, INT32U len, INT8S *notify);
#if 1
    extern INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, osMessageQId *os_q);
#else
    extern INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify);
#endif
extern INT32S drv_l1_adc_dbf_set(INT16S *data,INT32U len);
extern INT32S drv_l1_adc_dbf_status_get(void);
extern INT32S drv_l1_adc_dma_status_get(void);
extern void drv_l1_adc_dbf_free(void);
extern void drv_l1_adc_mic_enable(void);
extern void drv_l1_mic_lr_set(INT8U ch);
extern void drv_l1_audio_i2s_rx_path_set();
extern void drv_l1_adc_mic_enable(void);
extern void drv_l1_adc_mic_disable(void);
extern void drv_l1_aec_sign_channel(INT8U sign, INT8U ch);
extern void drv_l1_adc_dagc_enable(INT8U enable);
extern void drv_l1_adc_dagc_mode(INT8U peak_mode, INT8U zero_cross);
extern void drv_l1_noisedet_signal_threshold(INT8U enable, INT16U threshold);
extern void drv_l1_noisedet_noise_suppress(INT8U enable, INT16U threshold);
extern void drv_l1_noisedet_signal_frame_threshold(INT16U threshold);
extern void drv_l1_noisedet_noise_frame_threshold(INT16U threshold);
extern void drv_l1_noisedet_signal_count_threshold(INT16U threshold);
extern void drv_l1_noisedet_fast_signal_release(INT8U enable, INT16U threshold);
extern void drv_l1_noisedet_signal_fast_rampup(INT8U enable);
extern void drv_l1_noisedet_adjust_pga(INT8U enable);
extern void drv_l1_noisedet_mute_first(INT8U enable, INT16U cnt);
extern void drv_l1_noisedet_fix_gain(INT8U enable, INT8U gain);
extern void drv_l1_adc_dagc_updatefrequency(INT16U updatefrequency);
extern void drv_l1_adc_dagc_scale(INT8U att_scale, INT8U rel_scale);
extern void drv_l1_adc_high_pass(INT8U enable, INT8U newadhp);
extern void drv_l1_noisedet_suppress_speed(INT8U spd);
extern void drv_l1_adc_dagc_att_rel_time(INT8U att_time, INT8U rel_time);
extern void drv_l1_adc_mic_boost_pga(INT8U boost_gain, INT8U mic_pga);
extern void drv_l1_adc_dagc_threshold(INT16U threshold, INT8U auto_adj);
extern void drv_l1_adc_dagc_update_threshold(INT16U update_threshold);
extern INT32U drv_l1_adc_dagc_current_boost_gain(void);
extern void drv_l1_adc_dagc_boost_set(INT32U boost);
extern void drv_l1_dagc_boost_auto_control_init(INT16U gain_increase, INT16U att_cyc, INT16U rel_cyc);
extern void drv_l1_dagc_boost_auto_control_uninit();


extern void drv_l1_adc_vref_enable_set(BOOLEAN status);


#endif		// __DRV_L1_ADC_H__
