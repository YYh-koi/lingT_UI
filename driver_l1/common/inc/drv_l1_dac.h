#ifndef __drv_l1_DAC_H__
#define __drv_l1_DAC_H__

typedef enum
{
	DAC_ENABLED,
	DAC_DISABLED
} DAC_STATUS;

typedef enum
{
	DAC_CHA,
	DAC_CHB
} DAC_CHANNELS;

typedef struct
{
	INT16S *cha_data;
	INT16S *chb_data;
	INT32U cha_len;
	INT32U chb_len;
	INT32U cha_count;
	INT32U chb_count;
	INT8S *cha_notify;
	INT8S *chb_notify;
} DAC_CTRL;

extern void drv_l1_dac_init(INT8U vic_daca, INT8U vic_dacb);
extern void drv_l1_dac_uninit(INT8U vic_daca, INT8U vic_dacb);
extern void drv_l1_dac_enable_set(BOOLEAN status);
extern void drv_l1_dac_disable(void);
extern void drv_l1_dac_fifo_level_set(INT8U cha_level,INT8U chb_level);
extern void drv_l1_dac_data_signed_set(BOOLEAN flag);
extern void drv_l1_dac_mono_set(void);
extern void drv_l1_dac_stereo_set(void);
extern void drv_l1_dac_stereo_indi_buffer_set(void);
extern void drv_l1_dac_sample_rate_set(INT32U hz);

extern INT32S drv_l1_dac_dbf_dma_channel_get(void);
extern INT32S drv_l1_dac_cha_dbf_put_callback(INT16S *data,INT32U len, void (*dma_user_isr)(INT8U, INT8U));

/**
 * @brief   stop dac timer, stop dac output.
 * @param   none
 * @return 	none
 */
extern void drv_l1_dac_timer_stop(void);

/**
 * @brief   set dac output volume
 * @param   gain: 0 ~ 0x3F, default is 0x1F
 * @return 	none
 */
extern void drv_l1_dac_pga_set(INT8U gain);

/**
 * @brief   get dac output volume
 * @param   none
 * @return 	volume
 */
extern INT8U drv_l1_dac_pga_get(void);

/**
 * @brief   Set DAC VREF enable
 * @param   status - 1:enable, 0:disable
 * @return 	none
 */
extern void drv_l1_dac_vref_set(BOOLEAN status);

/**
 * @brief   set dac interrupt enable or disable
 * @param   ch: DAC_CHA or DAC_CHB
 * @param   status: DAC_ENABLED or DAC_DISABLE
 * @return 	none
 */
extern void drv_l1_dac_int_set(INT8U ch, INT8U status);

/**
 * @brief   put data to cha by cpu mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	none
 */
extern void drv_l1_dac_cha_data_put(INT16S *data,INT32U len, INT8S *notify);

/**
 * @brief   put data to chb by cpu mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	none
 */
extern void drv_l1_dac_chb_data_put(INT16S *data,INT32U len, INT8S *notify);

/**
 * @brief   put data to cha by dma mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
extern INT32S drv_l1_dac_cha_data_dma_put(INT16S *data,INT32U len, INT8S *notify);

/**
 * @brief   put data to chb by dma mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
extern INT32S drv_l1_dac_chb_data_dma_put(INT16S *data,INT32U len, INT8S *notify);

/**
 * @brief   put data to cha by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @param   os_q: os queue
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
#if _OPERATING_SYSTEM != _OS_NONE
extern INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, osMessageQId os_q);
#else
extern INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify);
#endif

/**
 * @brief   set data to cha by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @return 	STATUS_OK / STATUS_FAIL
 */
extern INT32S drv_l1_dac_cha_dbf_set(INT16S *data,INT32U len);

/**
 * @brief   get dac double buffer status
 * @param   none
 * @return 	1 is full, 0 is empty
 */
extern INT32S drv_l1_dac_dbf_status_get(void);

/**
 * @brief   get dac status
 * @param   none
 * @return 	1 is busy, 0 is idle
 */
extern INT32S drv_l1_dac_dma_status_get(void);

/**
 * @brief   release double buffer dma channel
 * @param   none
 * @return 	none
 */
extern void drv_l1_dac_dbf_free(void);


extern void drv_l1_dac_audop_speaker_en(BOOLEAN status);

#endif		// __drv_l1_DAC_H__
