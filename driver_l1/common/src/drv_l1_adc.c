/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/

#include "drv_l1.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// P_ADC_SETUP
#define ADC_ADBEN			(1 << 15)	//AD Bias Reference Voltage Enable
#define ADC_ADCEN			(1 << 14)	//ADC Enable
#define ADC_CLKSEL			(0x07 << 8)	//ADC Conversion Time Select
#define ADC_ASEN			(1 << 7) 	//Auto Sampling Mode Enable
#define ADC_AUTO_CH_SEL		(0x0870)	//Auto Sample Mode ADC channel selection
#define ADC_ASMEN			(1 << 3)	//ADC Auto Sampling Mode Enable
#define ADC_ASTMS			(0x07)		//ADC Auto Sampling Timer Selection

// P_ADC_MADC_CTRL
#define ADC_ADCRIF			(1 << 15)	//AD Conversion Ready Interrupt Flag & Clear
#define ADC_ADCRIEN			(1 << 14)	//AD Conversion Ready Interrupt Enable
#define ADC_CNVRDY			(1 << 7) 	//AD Conversion Ready
#define ADC_STRCNV			(1 << 6)	//Manual Start AD Conversion
#define ADC_MIASE			(1 << 5)	//AD Conversion Error Flag1
#define ADC_ASIME			(1 << 4)	//AD Conversion Error Flag2
#define ADC_MANUAL_CH_SEL	(0x0F)

// P_ADC_ASADC_CTRL
#define ADC_AUTO_ASIF		(1 << 15)	//Auto Sample Mode FIFO Full interrupt flag
#define ADC_AUTO_ASIEN		(1 << 14)	//Auto Sample Mode FIFO Full Interrupt Enable
#define ADC_ASFF			(1 << 13)	//Auto Sample Mode FIFO Full Flag
#define ADC_ASFOV			(1 << 12)	//Auto Sample Mode FIFO Overflow Flag
#define ADC_ASADC_DMA		(1 << 11)	//DMA mode
#define ADC_OVER			(1 << 10)	//Auto Sample FIFO Over Write Mode, 0:skip
#define ADC_FIFO_LEVEL		(0xF << 4) 	//FIFO level



#define ADC_ERR_WRONG_LEN  (-1)
#define ADC_BLOCK_LEN      2048

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		memset((INT8S *)ptr, 0x00, cblen)

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static ADC_CTRL adc_ctrl;
static DMA_STRUCT dma_adc_dbf;
static void (*adc_user_isr)(INT16U data);
void SARADC_IRQHandler(void);
void ADCF_IRQHandler(void);


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**
 * @brief   adc initialize.
 * @param   vic_adc / vic_adcf - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_adc_init(INT8U vic_adc, INT8U vic_adcf)
{
    drv_l1_clock_set_system_clk_en(CLK_EN2_SAR_ADC,1);

	R_ADC_SETUP = 0;
	R_ADC_ASADC_CTRL = 0;
	R_ADC_MADC_CTRL = 0;

	R_ADC_SETUP |= ADC_ADBEN; // AD Bias Reference Voltage Enable
	//R_ADC_USELINEIN = 0;
	R_ADC_SH_WAIT = 0x0807;

	adc_user_isr = 0;
	CLEAR(&adc_ctrl, sizeof(ADC_CTRL));
	CLEAR(&dma_adc_dbf, sizeof(DMA_STRUCT));
	dma_adc_dbf.channel = 0xff;

	vic_irq_register(VIC_ADC, SARADC_IRQHandler);
	vic_irq_enable(VIC_ADC);
    if (vic_adc) {
        vic_fiq_enable(VIC_ADC);
    }
	else {
		vic_fiq_disable(VIC_ADC);
	}
}
/**
 * @brief   adc un-initialize.
 * @param   vic_adc / vic_adcf !V 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_adc_uninit(INT8U vic_adc, INT8U vic_adcf)
{
	R_ADC_SETUP = 0;
	R_ADC_ASADC_CTRL = 0;
	R_ADC_MADC_CTRL = 0;

	R_ADC_SETUP &= ~ADC_ADBEN; // AD Bias Reference Voltage Enable

	adc_user_isr = 0;

    vic_irq_disable(VIC_ADC);
    vic_fiq_disable(VIC_ADC);

    vic_irq_disable(VIC_ADCF);
    vic_fiq_disable(VIC_ADCF);

    drv_l1_clock_set_system_clk_en(CLK_EN2_SAR_ADC,0);
}
/**
 * @brief   adc auto sample isr.
 * @param   none
 * @return 	none
 */
void ADCF_IRQHandler(void)
{
    while(((R_ADC_ASADC_CTRL & 0xF) != 0) && (adc_ctrl.count < adc_ctrl.data_len)) {
        adc_ctrl.adc_data[adc_ctrl.count++] = R_ADC_ASADC_DATA;
	}
	R_ADC_ASADC_CTRL |= ADC_AUTO_ASIF;  /* clear flag must after getting data */

	if (adc_ctrl.count >= adc_ctrl.data_len) {
		drv_l1_adc_auto_int_set(FALSE); /* disable fifo interrupt */
		*adc_ctrl.notify = 1;
	}
}
/**
 * @brief   adc manual sample isr.
 * @param   none
 * @return 	none
 */
void SARADC_IRQHandler(void)
{
	if (R_ADC_MADC_CTRL & ADC_ADCRIF) {
		if(R_ADC_MADC_CTRL & (ADC_MIASE | ADC_ASIME)) {
			drv_l1_adc_manual_sample_start();
			return;
		}
/*
		if((R_ADC_MADC_CTRL & ADC_CNVRDY) == 0) {
			drv_l1_adc_manual_sample_start();
			return;
		}
*/
	    R_ADC_MADC_CTRL |= ADC_ADCRIF;
	    if((*adc_user_isr))
	    //(*adc_user_isr)((INT16U)R_ADC_MADC_DATA);
	    (*adc_user_isr) (((INT16U) R_ADC_MADC_DATA & ~ADC_MANUAL_CH_SEL) | (( R_ADC_MADC_CTRL & ADC_MANUAL_CH_SEL  ) +1) ); //save data &  ADC channel
	    R_ADC_MADC_CTRL &= ~ADC_ADCRIEN; /* disable manual interrupt */
	}
}

//void drv_l1_adc_vref_enable_set(BOOLEAN status)  //GP329xxxA removed ADBEN

/**
 * @brief   set adc convert time.
 * @param   value[in]: 64xClk ~ 4096xClk
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_adc_conv_time_sel(INT32U value)
{
	INT32S ret = STATUS_OK;

	if (value > 5) {
		ret = STATUS_FAIL;
	} else {
		R_ADC_SETUP &= ~ADC_CLKSEL;
		R_ADC_SETUP |= (value<<8);
	}

	return ret;
}

/**
 * @brief   set adc fifo threshold.
 * @param   level[in]: 0 ~ 0xF
 * @return 	none
 */
void drv_l1_adc_fifo_level_set(INT8U level)
{
	R_ADC_ASADC_CTRL &= ~ADC_FIFO_LEVEL;
	R_ADC_ASADC_CTRL |= (level << 4);
}

/**
 * @brief   clear adc fifo
 * @param   none
 * @return 	none
 */
void drv_l1_adc_fifo_clear(void)
{
	INT16U dummy;
	while((R_ADC_ASADC_CTRL & 0xF) != 0) {
		dummy = R_ADC_ASADC_DATA;
		if(dummy){};	//for remove compiler warning
	}
}

/**
 * @brief   set adc auto channel number.
 * @param   ch[in]: 0 ~ 7
 * @return 	none
 */
void drv_l1_adc_auto_ch_set(INT8U ch)
{
	R_ADC_SETUP &= ~ADC_AUTO_CH_SEL;
	R_ADC_SETUP |= ((ch&0x7) << 4)|((ch&0x8)<<8);//ch[3:0], register [11][6:4]
	if(ch<8)
        R_LINEIN_CTRL |= 0x1<<ch;
}

/**
 * @brief   set adc manual channel number.
 * @param   ch[in]: 0 ~ 7
 * @return 	none
 */
void drv_l1_adc_manual_ch_set(INT8U ch)
{
	#if 0 //gpcvxx
	R_ADC_MADC_CTRL &= ~ADC_MANUAL_CH_SEL;
	R_ADC_MADC_CTRL |= ch;
	R_LINEIN_CTRL |= 0x1<<ch;
	#else
	//To decrease 1 (ch -1) , because ADC_LINE_0 = 1.
	R_ADC_MADC_CTRL &= ~ADC_MANUAL_CH_SEL;
	R_ADC_MADC_CTRL |= (ch -1);
    R_LINEIN_CTRL |= 0x1<<(ch -1);
	#endif
}

/**
 * @brief   set adc auto interrupt enable.
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_adc_auto_int_set(BOOLEAN status)
{
	if (status == TRUE) {
		R_ADC_ASADC_CTRL |= ADC_AUTO_ASIEN;
	} else {
		R_ADC_ASADC_CTRL &= ~ADC_AUTO_ASIEN;
	}
}

/**
 * @brief   set adc manual mode callback function.
 * @param   user_isr[in]: function
 * @return 	none
 */
void drv_l1_adc_manual_callback_set(void (*user_isr)(INT16U data))
{
	if (user_isr != 0) {
		adc_user_isr = user_isr;
	}
}

/**
 * @brief   set adc manual sample start.
 * @param   none
 * @return 	STATUS_OK
 */
INT32S drv_l1_adc_manual_sample_start(void)
{
	R_ADC_MADC_CTRL |= ADC_ADCRIF|ADC_MIASE|ADC_ASIME;	// clear flag
	R_ADC_MADC_CTRL |= ADC_ADCRIEN;	// enable interrupt
	R_ADC_MADC_CTRL |= ADC_STRCNV;	// start manual sample
	return STATUS_OK;
}

/**
 * @brief   set adc auto sample start.
 * @param   none
 * @return 	STATUS_OK
 */
INT32S drv_l1_adc_auto_sample_start(void)
{
	R_ADC_SETUP |= ADC_ADCEN|ADC_ADBEN|ADC_ASEN;
	R_ADC_ASADC_CTRL |= ADC_AUTO_ASIF;	// clear fifo interrupt flag
	R_ADC_ASADC_CTRL |= ADC_ASADC_DMA;	// DMA mode enable
	R_ADC_SETUP |= ADC_ASMEN;			// start auto sample
	return STATUS_OK;
}

/**
 * @brief   adc auto sample stop.
 * @param   none
 * @return 	none
 */
void drv_l1_adc_auto_sample_stop(void)
{
	R_ADC_SETUP &= ~ADC_ASMEN;
}

/**
 * @brief   adc sample rate and use timer set start.
 * @param   timer_id[in]: timerA ~ timerF
 * @param   hz[in]: sample rate
 * @return 	none
 */
INT32S drv_l1_adc_sample_rate_set(INT8U timer_id, INT32U hz)
{
	R_ADC_SETUP &= ~ADC_ASTMS;
	R_ADC_SETUP |= timer_id;
	return adc_timer_freq_setup(timer_id, hz);
}

/**
 * @brief   adc used timer set stop.
 * @param   timer_id[in]: timerA ~ timerF
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_timer_stop(INT8U timer_id)
{
	return timer_stop(timer_id);
}

/**
 * @brief   adc get data.
 * @param   data[in]: data pointer
 * @param   len[in]: length
 * @param   notify[in]: notify pointer
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_auto_data_get(INT16U *data, INT32U len, INT8S *notify)
{
	if (len > ADC_BLOCK_LEN) {
		return ADC_ERR_WRONG_LEN;
	}
	R_ADC_ASADC_CTRL &= ~ADC_ASADC_DMA; /* DMA mode disable */

	adc_ctrl.adc_data = data;
	adc_ctrl.data_len = len;
	adc_ctrl.count = 0;
	adc_ctrl.notify = notify;
	*adc_ctrl.notify = 0;

	if (len != 0) {
		drv_l1_adc_auto_int_set(TRUE); /* enable fifo interrupt */
	}
	return STATUS_OK;
}

/**
 * @brief   adc get data by dma.
 * @param   data[in]: data pointer
 * @param   len[in]: length
 * @param   notify[in]: notify pointer
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_auto_data_dma_get(INT16U *data, INT32U len, INT8S *notify)
{
	INT32S status;
	DMA_STRUCT dma_struct;

	if (len > ADC_BLOCK_LEN) {
		return ADC_ERR_WRONG_LEN;
	}

	*notify = C_DMA_STATUS_WAITING;
	dma_struct.s_addr = (INT32U) P_ADC_ASADC_DATA;
	dma_struct.t_addr = (INT32U) data;
	dma_struct.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct.count = (INT32U) len;
	dma_struct.notify = notify;
	dma_struct.timeout = 0;
	//dma_struct.aes = 0;
	dma_struct.trigger = 0;

	status = drv_l1_dma_transfer(&dma_struct);
	if (status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   get data from mic by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @param   os_q: os queue
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
#if _OPERATING_SYSTEM != _OS_NONE
INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, osMessageQId *os_q)
#else
INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify)
#endif
{
	INT32S status;

	dma_adc_dbf.s_addr = (INT32U) P_ADC_ASADC_DATA;
	dma_adc_dbf.t_addr = (INT32U) data;
	dma_adc_dbf.width = DMA_DATA_WIDTH_2BYTE;
	dma_adc_dbf.count = (INT32U) len;
	dma_adc_dbf.notify = NULL;
	dma_adc_dbf.timeout = 0;

#if _OPERATING_SYSTEM != _OS_NONE
	status = drv_l1_dma_transfer_with_double_buf(&dma_adc_dbf, (osMessageQId)os_q);
#else
	dma_adc_dbf.notify = notify;
	status = drv_l1_dma_transfer_with_double_buf(&dma_adc_dbf);
#endif
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   set data from adc by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_adc_dbf_set(INT16S *data,INT32U len)
{
	INT32S status;

	dma_adc_dbf.t_addr = (INT32U) data;
	dma_adc_dbf.count = (INT32U) len;
	dma_adc_dbf.notify = NULL;
	dma_adc_dbf.timeout = 0;

	status = drv_l1_dma_transfer_double_buf_set(&dma_adc_dbf);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   get adc dma double buffer status
 * @param   none
 * @return 	1 is full, 0 is empty
 */
INT32S drv_l1_adc_dbf_status_get(void)
{
	if (dma_adc_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_dbf_status_get(dma_adc_dbf.channel);
}

/**
 * @brief   get adc dma status
 * @param   none
 * @return 	1 is busy, 0 is idle
 */
INT32S drv_l1_adc_dma_status_get(void)
{
	if (dma_adc_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_status_get(dma_adc_dbf.channel);
}

/**
 * @brief   release double buffer dma channel
 * @param   none
 * @return 	none
 */
void drv_l1_adc_dbf_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&dma_adc_dbf);
	dma_adc_dbf.channel = 0xff;
}

//audio ADC
/**
 * @brief   set audio ADC high pass filter
 * @param   enable, 1:enable
 * @param   newadhp, 0:19bit HPF, 1:31bit HPF
 * @return 	none
 */
void drv_l1_adc_high_pass(INT8U enable, INT8U newadhp)
{
    if(enable == 1)
        R_MIC_MACRO |= (0x1<<5);
    else
        R_MIC_MACRO &= ~(0x1<<5);

    if(newadhp == 1)
        R_MAF_CNT |= (0x1<<2); //31bit
    else
        R_MAF_CNT &= ~(0x1<<2); //19bit
}
/**
 * @brief   set MIC enable
 * @param   none
 * @return 	none
 */
void drv_l1_adc_mic_enable(void)
{
    drv_l1_clock_set_system_clk_en(CLK_EN2_SAR_ADC, ENABLE);
    // check CODEC LDO enable
	if((R_SYSTEM_POWER_CTRL0 & 0x100) == 0) {
		R_SYSTEM_POWER_CTRL0 |= 0x700;
		//osDelay(200);
	}

	    R_MIC_MACRO &= ~(0xF);
	    R_MIC_MACRO |= 0x1;
	    R_MIC_MACRO |= 0x4;
	    //osDelay(350);//V30_CODEC capacity 1uF(IC side) + 1uF(MIC circuit)
}
/**
 * @brief   set MIC disable
 * @param   none
 * @return 	none
 */
void drv_l1_adc_mic_disable(void)
{
    // turn off MIC
	R_MIC_MACRO &= (~0xF);
	R_MIC_PGA = 0x001F;

	//check DAC whether is enable ? due to DAC & MIC share same LDO
    drv_l1_clock_set_system_clk_en(CLK_EN2_DAC, ENABLE);
    if(R_DAC_CHA_CTRL & (0x1<<12)/*DAC_VREFEN*/) {//DAC is enable, disable DAC by itself

    } else { //DAC is OFF
        //from drv_l1_dac_disable()
        /* at least delay 300 ms */
        drv_msec_wait(300);

        /* clear ENCI */
        R_ANALOG_CTRL = 0;

        drv_l1_clock_set_system_clk_en(CLK_EN2_DAC, DISABLE);
    }
}
/**
 * @brief   set MIC boost and PGA gain
 * @param   boost gain
    0x3 = 24dB
    0x2 = 16dB
    0x1 =  8dB
    0x0 =  0dB
 * @param   PGA gain
    PGAG	Gain(dB)	PGAG	Gain(dB)	PGAG	Gain(dB)	PGAG	Gain(dB)
    00000	33	        00001	31.5	    00010	30	        00011	28.5
    00100	27	        00101	25.5	    00110	24	        00111	22.5
    01000	21	        01001	19.5	    01010	18	        01011	16.5
    01100	15	        01101	13.5	    01110	12	        01111	10.5
    10000	9	        10001	7.5	        10010	6	        10011	4.5
    10100	3	        10101	1.5	        10110	0	        10111	-1.5
    11000	-3	        11001	-4.5	    11010	-6	        11011	-7.5
    11100	-9	        11101	-10.5	    11110	-12	        11111	-!U
 * @return 	none
 */
void drv_l1_adc_mic_boost_pga(INT8U boost_gain, INT8U mic_pga)
{

    R_MIC_PGA &= ~(0x301F);
    R_MIC_PGA |= (boost_gain<<12)|(mic_pga&0x1F);
}
/**
 * @brief   set DAGC mode
 * @param   peak_mode, 0:RMS, 1:peak
 * @param   zero_cross, 1:enable
 * @return 	none
 */
void drv_l1_adc_dagc_mode(INT8U peak_mode, INT8U zero_cross)
{
    R_DAGC_CTRL &= ~0x8008;
    R_DAGC_CTRL |= (peak_mode<<15)|(zero_cross<<3)|0x7;
}
/**
 * @brief   set DAGC threshold
 * @param   threshold, 0x1~0x7FFF
 * @param   auto_adj, 1:signal threshold auto adjust enable
 * @return 	none
 */
void drv_l1_adc_dagc_threshold(INT16U threshold, INT8U auto_adj)
{
    R_DAGC_THRESHOLD = ((auto_adj&0x1)<<15)|(0x7FFF&threshold);
}
/**
 * @brief   set DAGC scale
 * @param   att_scale
 * @param   rel_scale
 0: scale*1
 1: scale*4
 2: scale*16
 3: scale*64
 * @return 	none
 */
void drv_l1_adc_dagc_scale(INT8U att_scale, INT8U rel_scale)
{
    R_DAGC_CTRL &= ~(0xF<<4);
    R_DAGC_CTRL |=((0x3&att_scale)<<6)|((0x3&rel_scale)<<4);
}
/**
 * @brief   set DAGC attack and release time
 * @param   att_time, 0x0~0xFF
 * @param   rel_time, 0x0~0xFF
 * @return 	none
 */
void drv_l1_adc_dagc_att_rel_time(INT8U att_time, INT8U rel_time)
{
    R_DAGC_TIME =(att_time<<8)|(rel_time);
}
/**
 * @brief   set DAGC enable
 * @param   enable, 1:enable
 * @return 	none
 */
void drv_l1_adc_dagc_enable(INT8U enable)
{
    if(enable==1)
        R_DAGC_ENABLE |= 0x1;
    else
        R_DAGC_ENABLE &=~0x1;
}
/**
 * @brief   set DAGC update frequency
 * @param   updatefrequency, 0~0xFFF
 * @return 	none
 */
void drv_l1_adc_dagc_updatefrequency(INT16U updatefrequency)
{
    R_DAGC_ENABLE &= ~(0xFFF<<4);
    R_DAGC_ENABLE |= updatefrequency<<4;
}
/**
 * @brief   set DAGC update threshold
 * @param   update_threshold, 0~0x7FFF
 * @return 	none
 */
void drv_l1_adc_dagc_update_threshold(INT16U update_threshold)
{
    R_DAGC_UTHRES &= ~(0x7FFF);
    R_DAGC_UTHRES |= update_threshold&0x7fff;
}
/**
 * @brief   get DAGC PGA gain
 * @param   none
 * @return 	PGA gain
 */
INT32U drv_l1_adc_dagc_current_pga_gain(void)
{
    //0xx0 :33  dB
    //0xx1 :31.5dB
    //0xx2 :30  dB
    //
    // each step 1.5dB
    //
    //0x1F = -oo
    return ((R_MIC_PGA&0x03E0)>>5);
}
/**
 * @brief   set mic interrupt
 * @param   enable, 1:enable
 * @param   max: MIC_MAX
 * @param   min: MIC_MIN
 * interrupt triggered when
 * ABS(mic_data) > MIC_MAX reach MIC_CNT_MAX times during a frame
 * ABS(mic_data) < MIC_MIN last (MIC_CNT_MAX *8) of frames
 * @return 	none
 */
void drv_l1_adc_mic_inten_set(INT8U enable, INT16U max, INT16U min)
{
    R_MIC_INTEN &= ~0x8000;
    R_MIC_INTEN |= (enable&0x1)<<15;
    R_MIC_MAX = max&0x7FFF; //[14:0]
    R_MIC_MIN = min&0x7FFF;
}
/**
 * @brief   set MIC LR set
 * @param   ch :LR
 * @return 	none
 */
void drv_l1_mic_lr_set(INT8U ch)
{
    if(ch==1)
        R_MIC_PGA |= 0x1<<14;
    else
        R_MIC_PGA &=~(0x1<<14);
}
/**
 * @brief   set noise detection mute at first
 * @param   enable, 1:enable
 * @param   cnt, 0~0x1FFF
 * @return 	none
 */
void drv_l1_noisedet_mute_first(INT8U enable, INT16U cnt)
{
    R_MAF_CNT &= ~(0x1FFF<<3);
    R_MAF_CNT |= (cnt<<3);

    R_NOISEDET_CTRL3 &= ~(0x1<<12);
    R_NOISEDET_CTRL3 |= ((enable&0x1)<<12);
}
/**
 * @brief   set noise detection signal theshold
 * @param   enable, 1:enable
 * @param   threshold, 0~0x7FFF
 * @return 	none
 */
void drv_l1_noisedet_signal_threshold(INT8U enable, INT16U threshold)
{
    R_NOISEDET_CTRL0 = ((enable&0x1)<<15) | (threshold&0x7FFF);
}
/**
 * @brief   set noise detection signal frame theshold
 * @param   threshold, 0~0xF
 * @return 	none
 */
void drv_l1_noisedet_signal_frame_threshold(INT16U threshold)
{
    R_NOISEDET_CTRL1 &= ~(0xF);
    R_NOISEDET_CTRL1 |= (threshold&0xF);
}
/**
 * @brief   set noise detection noise frame theshold
 * @param   threshold, 0~0xFFF
 * @return 	none
 */
void drv_l1_noisedet_noise_frame_threshold(INT16U threshold)
{
    R_NOISEDET_CTRL1 &= ~(0xFFF<<4);
    R_NOISEDET_CTRL1 |= (threshold&0xFFF)<<4;
}
/**
 * @brief   set noise detection noise suppress theshold
 * @param   enable, 1:enable
 * @param   threshold, 0~0xF
 * @return 	none
 */
void drv_l1_noisedet_noise_suppress(INT8U enable, INT16U threshold)
{
    R_NOISEDET_CTRL2 &= ~(0xF<<12);
    R_NOISEDET_CTRL2 |= (threshold&0xF)<<12;

    R_NOISEDET_CTRL3 &= ~(0x1<<15);
    R_NOISEDET_CTRL3 |= (enable&0x1)<<15;
}
/**
 * @brief   set noise detection signal count theshold
 * @param   threshold, 0~0xFFF
 * @return 	none
 */
void drv_l1_noisedet_signal_count_threshold(INT16U threshold)
{
    R_NOISEDET_CTRL2 &= ~(0xFFF);
    R_NOISEDET_CTRL2 |= (threshold&0xFFF);
}
/**
 * @brief   set noise detection fast signal release
 * @param   enable, 1:enable
 * @param   threshold, 0~0xFF
 * @return 	none
 */
void drv_l1_noisedet_fast_signal_release(INT8U enable, INT16U threshold)
{
    R_NOISEDET_CTRL3 &= ~(0xFF);
    R_NOISEDET_CTRL3 |= (threshold&0xFF);

    R_NOISEDET_CTRL3 &= ~(0x1<<14);
    R_NOISEDET_CTRL3 |= (enable&0x1)<<14;
}
/**
 * @brief   set noise detection fast ramp up enable
 * @param   enable, 1:enable
 * @return 	none
 */
void drv_l1_noisedet_signal_fast_rampup(INT8U enable)
{
    R_DAGC_ENABLE &= ~(0x1<<1);
    R_DAGC_ENABLE |= ((enable&0x1)<<1);
}
/**
 * @brief   set PGA adjust when noise is detected
 * @param   enable, 1:enable
 * @return 	none
 */
void drv_l1_noisedet_adjust_pga(INT8U enable)
{
    if(enable)
        R_NOISEDET_CTRL3 |= 0x1<<9;
    else
        R_NOISEDET_CTRL3 &= ~(0x1<<9);
}
/**
 * @brief   set noise detection hold and release set
 * @param   enable, 1:enable
 * @param   hold : hold # of frame count (count by update frequency) when no gain chnaged
 * @param   release : release speed # of update frequence
 * @return 	none
 */
void drv_l1_noisedet_hold_release(INT8U enable, INT16U hold, INT16U release)
{
    if(enable) {
        R_NOISEDET_CTRL3 |= 0x1<<8;
        R_DAGC_HOLD_CYC = hold;
        R_DAGC_RELEASE_CYC = release;
    } else {
        R_NOISEDET_CTRL3 &= ~(0x1<<8);
        R_DAGC_HOLD_CYC = 0;
        R_DAGC_RELEASE_CYC = 0;
    }
}
/**
 * @brief   set noise detection suppress speed
 * @param   spd - 0: normal,    1: 2x slower
				  2: 3x slower, 3: 4x slower
				  4: 5x slower, 5: 6x slower
				  6: 7x slower, 8: 4x slower
 * @return 	none
 */
void drv_l1_noisedet_suppress_speed(INT8U spd)
{
    R_DAGC_CTRL &= ~(0x7<<8);
    R_DAGC_CTRL |= ((spd&0x7)<<8);
}
/**
 * @brief   set audio i2s path
 * @param   none
 * @return 	none
 */
void drv_l1_audio_i2s_rx_path_set()
{
    R_SYSTEM_CODEC_CTRL1 &= ~(0x1<<14);
}
/**
 * @brief   set noise detection fix gain, ignore DAGC function
 * @param   enable, 1:enable
 * @param   gain
 * @return 	none
 */
void drv_l1_noisedet_fix_gain(INT8U enable, INT8U gain)
{
    R_DIGITAL_GAIN &= ~((0x1<<7)|(0x1F<<0));
    R_DIGITAL_GAIN |= ((enable&0x1)<<7) | (gain&0x1F);
}
/**
 * @brief   set aec sign and channel
 * @param   sign, 1:signed dac data
 * @param   ch: 0:left data
 * @return 	none
 */
void drv_l1_aec_sign_channel(INT8U sign, INT8U ch)
{
    if(sign) {
        R_DIGITAL_GAIN |= (0x1<<11);
    } else {
        R_DIGITAL_GAIN &= ~(0x1<<11);
    }
    if(ch) {
        R_DIGITAL_GAIN |= (0x1<<10);
    } else {
        R_DIGITAL_GAIN &= ~(0x1<<10);
    }
}

/**
 * @brief   get DAGC Boost gain
 * @param   none
 * @return 	Boost gain
 */
INT32U drv_l1_adc_dagc_current_boost_gain(void)
{
    return ((R_MIC_PGA&0x3000)>>12);
}
/**
 * @brief   get DAGC PGA gain
 * @param   none
 * @return 	PGA gain
 */
void drv_l1_adc_dagc_boost_set(INT32U boost)
{
    INT32U cur_boost = R_MIC_PGA;
    cur_boost &= ~(0x3<<12);
    cur_boost |= (boost&0x3)<<12;
    R_MIC_PGA = cur_boost;
}

//#define DAGC_AUTO_BOOST_TIMER   TIMER_G
static INT16U attack_count, attack_cycle;
static INT16U release_count, release_cycle;
static INT8U delay_boost = 0, count_delay = 0;
static INT16U gain_increase_threshold;
static INT8U max_boost;
static xTimerHandle dagc_boost_timer;

static void boost_control_isr()
{
    INT32U cur_boost = drv_l1_adc_dagc_current_boost_gain();

    if(delay_boost == 0) {
        //DBG_PRINT("[%d]", drv_l1_adc_dagc_current_pga_gain());
        if(drv_l1_adc_dagc_current_pga_gain() >= 0x1E){ //gain is lowest
            attack_count++;
            if(attack_count >= attack_cycle){
                attack_count = 0;
                release_cycle = 0;
                if(cur_boost > 0){
                    cur_boost--;
                    drv_l1_adc_dagc_boost_set(cur_boost);
                    delay_boost = 1;
                    //DBG_PRINT("-%d\r\n", cur_boost);
                }
            }
            if(release_count>0){
                //release_count--;
                release_count = 0;
                //DBG_PRINT("r");
            }

        } else if( drv_l1_adc_dagc_current_pga_gain() <= (gain_increase_threshold) ){
            release_count++;
            if(release_count >= release_cycle){
                release_cycle = 0;
                attack_count = 0;
                if(cur_boost < max_boost){
                    cur_boost++;
                    drv_l1_adc_dagc_boost_set(cur_boost);
                    delay_boost = 1;
                    //DBG_PRINT("+%d\r\n", cur_boost);
                }
            }
            if(attack_count>0){
                //attack_count--;
                attack_count = 0;
                //DBG_PRINT("a");
            }
        }
    } else {
        count_delay++;
        if(count_delay >= 50) {
            count_delay = 0;
            delay_boost = 0;
        }

    }
    //DBG_PRINT("%d",delay_boost);
}

void drv_l1_dagc_boost_auto_control_init(INT16U gain_increase, INT16U att_cyc, INT16U rel_cyc)
{
    gain_increase_threshold = gain_increase&0x1F;
    attack_cycle = att_cyc;
    release_cycle = rel_cyc;
    attack_count = 0;
    release_count = 0;
    count_delay = 0;
    delay_boost = 0;
    max_boost = drv_l1_adc_dagc_current_boost_gain();
    //timer_msec_setup(DAGC_AUTO_BOOST_TIMER, 5, 0, boost_control_isr);
#if _OPERATING_SYSTEM == _OS_FREERTOS
    dagc_boost_timer = xTimerCreate("DAGC_TIMER", 5/portTICK_RATE_MS, pdTRUE, NULL, boost_control_isr);
    if (dagc_boost_timer != NULL)
        xTimerStart(dagc_boost_timer, 0);
#endif
}

void drv_l1_dagc_boost_auto_control_uninit()
{
#if _OPERATING_SYSTEM == _OS_FREERTOS
    if (dagc_boost_timer != NULL) {
        xTimerStop(dagc_boost_timer, 0);
        xTimerDelete(dagc_boost_timer, 0);
    }
#endif
    //timer_stop(DAGC_AUTO_BOOST_TIMER);
}

void drv_l1_adc_vref_enable_set(BOOLEAN status)
{
	if(status)
	{
		R_ADC_SETUP |= ADC_ADBEN;	// AD Bias Reference Voltage Enable
	}
	else
	{
		R_ADC_SETUP &= ~ADC_ADBEN;	// AD Bias Reference Voltage Disable
	}
}

