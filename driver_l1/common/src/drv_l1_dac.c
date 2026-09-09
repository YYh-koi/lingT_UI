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
#define DAC_FEM_IF     (1 << 15)  /* DAC FIFO empty interrupt flag */
#define DAC_FEM_IEN    (1 << 14)  /* FIFO empty interrupt enable */
#define DAC_CH_EN      (1 << 13)  /* CHA,B enable */
#define DAC_VREFEN     (1 << 12)  /* DAC VREF control pin enable */
#define DAC_SIGNED     (1 << 11)  /* signed data */


#define DAC_CHB_SSF    (1 << 12)  /* CHB service frequency */
#define DAC_CHB_CHACFG (1 << 11)  /* CHB use CHA's configuration */
#define DAC_MONO       (1 << 10)  /* Mono mode */

#define DAC_FIFO_FULL  (1 << 15)  /* FIFO full flag */
#define DAC_FIFO_RST   (1 << 8)   /* FIFO reset */
#define DAC_FIFO_LEVEL (0xF << 4) /* FIFO level */

#define DAC_MUTE       (1 << 8)   /* mute */
#define DAC_PGA        (0x3F << 0) /* DAC PGA gain */

#define DAC_IIS_EN     (1 << 0)   /* IIS enable */

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		memset((void *)ptr, 0x00, cblen)

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT32U fps_audio_dac_dbf_end;

static DAC_CTRL dac_ctrl;
static INT8U cha_int_level;
static INT8U chb_int_level;
static DMA_STRUCT dma_struct_dbf;
static DMA_STRUCT dma_struct_dbf_b;
void AUDA_IRQHandler(void);
void AUDB_IRQHandler(void);

static void drv_l1_dac_msec_wait(INT32U msec)
{
	drv_msec_wait(msec);
}

/**
 * @brief   set dac initialize
 * @param   vic_daca  - 0:IRQ; other values:FIQ
 * @param   vic_dacb  - 0:IRQ; other values:FIQ
 * @return 	none
 */
void drv_l1_dac_init(INT8U vic_daca, INT8U vic_dacb)
{
    drv_l1_clock_set_system_clk_en(CLK_EN2_DAC,1);
    //enable codec LDO
    R_SYSTEM_POWER_CTRL0 |= 0x1<<8;

	R_DAC_CHA_CTRL = 0;
	R_DAC_CHB_CTRL = 0;

	R_DAC_CHA_CTRL |= DAC_FEM_IF; /* clear FIFO empty flag */
	R_DAC_CHB_CTRL |= DAC_FEM_IF;

	R_DAC_CHA_CTRL &= ~DAC_FEM_IEN; /* disabe FIFO empty interrupt */
	R_DAC_CHB_CTRL &= ~DAC_FEM_IEN; /* disabe FIFO empty interrupt */

	R_DAC_CHA_CTRL |= DAC_VREFEN;
	while((R_DAC_PGA & DAC_MUTE) != 0); /* wait vref ready */

	R_DAC_CHA_CTRL |= (DAC_CH_EN|DAC_SIGNED); /* enable CHA and use signed data type */
	R_DAC_CHB_CTRL |= (DAC_CH_EN|DAC_CHB_SSF);

	R_DAC_CHA_FIFO |= DAC_FIFO_RST; /* reset FIFO */
	R_DAC_CHB_FIFO |= DAC_FIFO_RST;

    vic_irq_register(VIC_AUDA, AUDA_IRQHandler);
    vic_irq_enable(VIC_AUDA);
    vic_irq_register(VIC_AUDB, AUDB_IRQHandler);
    vic_irq_enable(VIC_AUDB);
    if(vic_daca){
        vic_fiq_enable(VIC_AUDA);
    } else {
		vic_fiq_disable(VIC_AUDA);
    }
    if(vic_dacb){
        vic_fiq_enable(VIC_AUDB);
    } else {
		vic_fiq_disable(VIC_AUDB);
    }

	drv_l1_dac_fifo_level_set(0x8,0x8);
	CLEAR(&dma_struct_dbf, sizeof(DMA_STRUCT));
	dma_struct_dbf.channel = 0xff;
}

/**
 * @brief   set dac uninitial
 * @param   vic_daca  - 0:IRQ, 1~4:FIQ number
 * @param   vic_dacb  - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_dac_uninit(INT8U vic_daca, INT8U vic_dacb)
{
	R_DAC_CHA_CTRL = 0;
	R_DAC_CHB_CTRL = 0;

	R_DAC_CHA_CTRL |= DAC_FEM_IF; /* clear FIFO empty flag */
	R_DAC_CHB_CTRL |= DAC_FEM_IF;

	R_DAC_CHA_CTRL &= ~DAC_FEM_IEN; /* disabe FIFO empty interrupt */
	R_DAC_CHB_CTRL &= ~DAC_FEM_IEN; /* disabe FIFO empty interrupt */

	drv_l1_dac_disable();

	R_DAC_CHA_FIFO |= DAC_FIFO_RST; /* reset FIFO */
	R_DAC_CHB_FIFO |= DAC_FIFO_RST;

    vic_irq_disable(VIC_AUDA);
    vic_fiq_disable(VIC_AUDA);
    vic_irq_disable(VIC_AUDB);
    vic_fiq_disable(VIC_AUDA);

	drv_l1_clock_set_system_clk_en(CLK_EN2_DAC,0);
}

/**
 * @brief   set dac enable
 * @param   status: enable / disable
 * @return 	none
 */
void drv_l1_dac_enable_set(BOOLEAN status)
{
	if (status == TRUE) {
		R_DAC_CHA_CTRL |= DAC_CH_EN;
		R_DAC_CHB_CTRL |= DAC_CH_EN;
	} else {
		R_DAC_CHA_CTRL &= ~DAC_CH_EN;
		R_DAC_CHB_CTRL &= ~DAC_CH_EN;
	}
}

/**
 * @brief   set dac disable
 * @param   none
 * @return 	none
 */
void drv_l1_dac_disable(void)
{
	/* disable OEB */
	R_DAC_CHA_CTRL &= ~DAC_CH_EN;
	R_DAC_CHB_CTRL &= ~DAC_CH_EN;

	/* disable ENI */
	R_DAC_CHA_CTRL &= ~DAC_VREFEN;

	/* wait ENC go low, about 170ms */
	while((R_ANALOG_CTRL & (1<<7)) != 0);

	/* at least delay 300 ms */
	drv_l1_dac_msec_wait(300);

	/* clear ENCI */
	R_ANALOG_CTRL = 0;
}

/**
 * @brief   set dac fifo threshold
 * @param   cha_level: cha fifo threshold
 * @param   chb_level: chb fifo threshold
 * @return 	none
 */
void drv_l1_dac_fifo_level_set(INT8U cha_level,INT8U chb_level)
{
	R_DAC_CHA_FIFO |= DAC_FIFO_RST; /* reset FIFO */
	R_DAC_CHB_FIFO |= DAC_FIFO_RST;

	R_DAC_CHA_FIFO &= ~DAC_FIFO_LEVEL;
	R_DAC_CHB_FIFO &= ~DAC_FIFO_LEVEL;

	R_DAC_CHA_FIFO |= (cha_level << 4);
	R_DAC_CHB_FIFO |= (cha_level << 4);
	cha_int_level = cha_level;
	chb_int_level = chb_level;
}

/**
 * @brief   set dac input is signed or un-signed data
 * @param   flag: signed or unsigned data
 * @return 	none
 */
void drv_l1_dac_data_signed_set(BOOLEAN flag)
{
	if (flag == TRUE) {
		R_DAC_CHA_CTRL |= DAC_SIGNED;
	}
	else {
		R_DAC_CHA_CTRL &= ~DAC_SIGNED;
	}
}

/**
 * @brief   set dac is mono
 * @param   none
 * @return 	none
 */
void drv_l1_dac_mono_set(void)
{
	R_DAC_CHB_CTRL |= DAC_MONO;
	R_DAC_CHB_CTRL |= DAC_CHB_CHACFG;
}

/**
 * @brief   set dac is stereo, left right left..data in buffer
 * @param   none
 * @return 	none
 */
void drv_l1_dac_stereo_set(void)
{
	R_DAC_CHB_CTRL &= ~DAC_MONO;
	R_DAC_CHB_CTRL |= DAC_CHB_CHACFG;
}

/**
 * @brief   set cha and chb input data individually
 * @param   none
 * @return 	none
 */
void drv_l1_dac_stereo_indi_buffer_set(void)
{
	R_DAC_CHB_CTRL &= ~DAC_MONO;
	R_DAC_CHB_CTRL &= ~DAC_CHB_CHACFG;
}

/**
 * @brief   set dac output sample rate
 * @param   hz: play speed
 * @return 	none
 */
void drv_l1_dac_sample_rate_set(INT32U hz)
{
	dac_timer_freq_setup(hz*2);
}

/**
 * @brief   stop dac timer, stop dac output.
 * @param   none
 * @return 	none
 */
void drv_l1_dac_timer_stop(void)
{
	timer_stop(TIMER_E);
}

/**
 * @brief   set dac output volume
 * @param   gain: 0 ~ 0x3F, default is 0x1F
 * @return 	none
 */
void drv_l1_dac_pga_set(INT8U gain)
{
	R_DAC_PGA &= ~DAC_PGA;
	R_DAC_PGA |= gain;
}

/**
 * @brief   get dac output volume
 * @param   none
 * @return 	volume
 */
INT8U drv_l1_dac_pga_get(void)
{
	INT8U pga;

	pga = R_DAC_PGA;
	pga &= DAC_PGA;

	return pga;
}

/**
 * @brief   Set DAC VREF enable
 * @param   status - 1:enable, 0:disable
 * @return 	none
 */
void drv_l1_dac_vref_set(BOOLEAN status)
{
	if (status == TRUE) { /* enable */
		R_DAC_CHA_CTRL |= DAC_VREFEN;
		while((R_DAC_PGA & DAC_MUTE) != 0); /* wait vref ready */
	}
	else {/* disable */
		R_DAC_CHA_CTRL &= ~DAC_VREFEN;
	}
}

/**
 * @brief   set dac interrupt enable or disable
 * @param   ch: DAC_CHA or DAC_CHB
 * @param   status: DAC_ENABLED or DAC_DISABLE
 * @return 	none
 */
void drv_l1_dac_int_set(INT8U ch, INT8U status)
{
	if (ch == DAC_CHA) {
		if (status == DAC_ENABLED) {
			R_DAC_CHA_CTRL |= DAC_FEM_IEN; /* enable FIFO empty interrupt */
		}
		else {
			R_DAC_CHA_CTRL &= ~DAC_FEM_IEN; /* disable FIFO empty interrupt */
		}
	}
	else {
		if (status == DAC_ENABLED) {
			R_DAC_CHB_CTRL |= DAC_FEM_IEN; /* enable FIFO empty interrupt */
		}
		else {
			R_DAC_CHB_CTRL &= ~DAC_FEM_IEN; /* disable FIFO empty interrupt */
		}
	}
}

void AUDA_IRQHandler(void)
{
	INT8U i = 0;

	R_DAC_CHA_CTRL |= DAC_FEM_IF; /* clear interrupt flag */

	/* start transmition */
    for (i=0;(i<(16-cha_int_level)) && (dac_ctrl.cha_len > dac_ctrl.cha_count);i++) {
		if((!(R_DAC_CHA_FIFO & DAC_FIFO_FULL))) {
			R_DAC_CHA_DATA = dac_ctrl.cha_data[dac_ctrl.cha_count++];
		}
	}

	if (dac_ctrl.cha_count >= dac_ctrl.cha_len) {
		drv_l1_dac_int_set(DAC_CHA, DAC_DISABLED);
		*dac_ctrl.cha_notify = 1;
	}
}

void AUDB_IRQHandler(void)
{
	INT8U i = 0;

	R_DAC_CHB_CTRL |= DAC_FEM_IF; /* clear interrupt flag */

	/* start transmition */
    for (i=0;(i<(16-chb_int_level)) && (dac_ctrl.chb_len > dac_ctrl.chb_count);i++) {
		if((!(R_DAC_CHB_FIFO & DAC_FIFO_FULL))) {
			R_DAC_CHB_DATA = dac_ctrl.chb_data[dac_ctrl.chb_count++];
		}
	}

	if (dac_ctrl.chb_count >= dac_ctrl.chb_len) {
		drv_l1_dac_int_set(DAC_CHB, DAC_DISABLED);
		*dac_ctrl.chb_notify = 1;
	}
}

/**
 * @brief   put data to cha by cpu mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	none
 */
void drv_l1_dac_cha_data_put(INT16S *data,INT32U len, INT8S *notify)
{
	dac_ctrl.cha_data = data;
	dac_ctrl.cha_len = len;
	dac_ctrl.cha_count = 0;
	dac_ctrl.cha_notify = notify;

	if (len != 0) {
		drv_l1_dac_int_set(DAC_CHA, DAC_ENABLED);
	}
}

/**
 * @brief   put data to chb by cpu mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	none
 */
void drv_l1_dac_chb_data_put(INT16S *data,INT32U len, INT8S *notify)
{
	dac_ctrl.chb_data = data;
	dac_ctrl.chb_len = len;
	dac_ctrl.chb_count = 0;
	dac_ctrl.chb_notify = notify;

	if (len != 0) {
		drv_l1_dac_int_set(DAC_CHB, DAC_ENABLED);
	}
}

/**
 * @brief   put data to cha by dma mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_dac_cha_data_dma_put(INT16S *data,INT32U len, INT8S *notify)
{
	DMA_STRUCT dma_struct={0};
	INT32S status;

	*notify = C_DMA_STATUS_WAITING;
	dma_struct.s_addr = (INT32U) data;
	dma_struct.t_addr = (INT32U) P_DAC_CHA_DATA;
	dma_struct.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct.count = (INT32U) len;
	dma_struct.notify = notify;
	dma_struct.timeout = 0;
	dma_struct.trigger = 0;

	status = drv_l1_dma_transfer(&dma_struct);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   put data to chb by dma mode
 * @param   data: data pointer
 * @param   len: length
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_dac_chb_data_dma_put(INT16S *data,INT32U len, INT8S *notify)
{
	DMA_STRUCT dma_struct={0};
	INT32S status;

	*notify = C_DMA_STATUS_WAITING;
	dma_struct.s_addr = (INT32U) data;
	dma_struct.t_addr = (INT32U) P_DAC_CHB_DATA;
	dma_struct.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct.count = (INT32U) len;
	dma_struct.notify = notify;
	dma_struct.timeout = 0;
	dma_struct.trigger = 0;

	status = drv_l1_dma_transfer(&dma_struct);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   put data to cha by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @param   os_q: os queue
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
#if _OPERATING_SYSTEM != _OS_NONE
INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, osMessageQId os_q)
#else
INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify)
#endif
{
	INT32S status;

	dma_struct_dbf.s_addr = (INT32U) data;
	dma_struct_dbf.t_addr = (INT32U) P_DAC_CHA_DATA;
	dma_struct_dbf.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct_dbf.count = (INT32U) len;
	dma_struct_dbf.notify = NULL;
	dma_struct_dbf.timeout = 0;

#if _OPERATING_SYSTEM != _OS_NONE
	status = drv_l1_dma_transfer_with_double_buf(&dma_struct_dbf, os_q);
#else
	dma_struct_dbf.notify = notify;
	status = drv_l1_dma_transfer_with_double_buf(&dma_struct_dbf);
#endif
	drv_l1_dma_ce_mode(0);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   set data to cha by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_dac_cha_dbf_set(INT16S *data,INT32U len)
{
	INT32S status;

	dma_struct_dbf.s_addr = (INT32U) data;
	dma_struct_dbf.count = (INT32U) len;
	dma_struct_dbf.notify = NULL;
	dma_struct_dbf.timeout = 0;
	status = drv_l1_dma_transfer_double_buf_set(&dma_struct_dbf);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   put data to chB by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @param   os_q: os queue
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
#if _OPERATING_SYSTEM != _OS_NONE
INT32S drv_l1_dac_chb_dbf_put(INT16S *data,INT32U len, osMessageQId os_q)
#else
INT32S drv_l1_dac_chb_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify)
#endif
{
	INT32S status;

	dma_struct_dbf_b.s_addr = (INT32U) data;
	dma_struct_dbf_b.t_addr = (INT32U) P_DAC_CHB_DATA;
	dma_struct_dbf_b.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct_dbf_b.count = (INT32U) len;
	dma_struct_dbf_b.notify = NULL;
	dma_struct_dbf_b.timeout = 0;

#if _OPERATING_SYSTEM != _OS_NONE
	status = drv_l1_dma_transfer_with_double_buf(&dma_struct_dbf_b, os_q);
#else
	dma_struct_dbf_b.notify = notify;
	status = drv_l1_dma_transfer_with_double_buf(&dma_struct_dbf_b);
#endif
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   set data to chB by dma with double buffer mode
 * @param   data: data pointer
 * @param   len: length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_dac_chb_dbf_set(INT16S *data,INT32U len)
{
	INT32S status;

	dma_struct_dbf_b.s_addr = (INT32U) data;
	dma_struct_dbf_b.count = (INT32U) len;
	dma_struct_dbf_b.notify = NULL;
	dma_struct_dbf_b.timeout = 0;
	status = drv_l1_dma_transfer_double_buf_set(&dma_struct_dbf_b);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   get dac double buffer status
 * @param   none
 * @return 	1 is full, 0 is empty
 */
INT32S drv_l1_dac_dbf_status_get(void)
{
	if (dma_struct_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_dbf_status_get(dma_struct_dbf.channel);
}

/**
 * @brief   get dac status
 * @param   none
 * @return 	1 is busy, 0 is idle
 */
INT32S drv_l1_dac_dma_status_get(void)
{
	if (dma_struct_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_status_get(dma_struct_dbf.channel);
}

/**
 * @brief   release double buffer dma channel
 * @param   none
 * @return 	none
 */
void drv_l1_dac_dbf_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&dma_struct_dbf);
	dma_struct_dbf.channel = 0xff;
}

/* === Turnkey === */
INT32S drv_l1_dac_cha_dbf_put_callback(INT16S *data,INT32U len, void (*dma_user_isr)(INT8U, INT8U))
{
	INT32S status;

	dma_struct_dbf.s_addr = (INT32U) data;
	dma_struct_dbf.t_addr = (INT32U) P_DAC_CHA_DATA;
	dma_struct_dbf.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct_dbf.count = (INT32U) len;
	dma_struct_dbf.notify = NULL;
	dma_struct_dbf.timeout = 0;

	status = drv_l1_dma_transfer_double_buf_with_callback(&dma_struct_dbf, NULL, dma_user_isr);
	drv_l1_dma_ce_mode(0);
	if(status != 0) {
		return status;
	}

	return STATUS_OK;
}

INT32S drv_l1_dac_dbf_dma_channel_get(void)
{
	if (dma_struct_dbf.channel == 0xff) {
		return -1;
	}
	return dma_struct_dbf.channel;
}

void drv_l1_dac_audop_speaker_en(BOOLEAN status)
{
	if (status)
	{
		R_DAC_CHA_CTRL &= ~0x40;
	}
	else
	{
		R_DAC_CHA_CTRL |= 0x40;
	}
}

