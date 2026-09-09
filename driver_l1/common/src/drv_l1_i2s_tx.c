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
 #define C_I2S_TX_USE_FIQ		0           // set this to 1 to use I2S interrupt in fast interrupt mode

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

 /**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 void I2STX_IRQHandler(void);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct i2sTxIsr_s
{
    void (*isr_func)(INT32U channel, INT32U isr_global);
    INT32U isr_global;
} i2sTxIsr_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static DMA_STRUCT i2s_tx_dma_dbf[4] = {{0}, {0}, {0}, {0}};
static INT8U i2s_tx_data_mode_list[] = {16, 18, 20, 22, 24, 32, 8, 32};
static INT8U i2s_tx_master_frame_size_list[] = {16, 24, 32, 48, 64, 96, 128, 176, 192};
static INT8U i2s_tx_slave_frame_size_list[] = {32};
static INT16U i2s_tx_spu_fs_list[] = {256, 384, 512, 768}; // 0:256 FS, 1:384 FS, 2:512 FS, 3:768 FS
static i2sTxIsr_t tx_isr[4]={{0,0}, {0,0}, {0,0}, {0,0}};
static INT8U tx_isr_round_start = 0;
static volatile i2sTxReg_t *tx_i2s_regs_list[4] = {(i2sTxReg_t *)I2S0_BASE, (i2sTxReg_t *)I2S1_BASE, (i2sTxReg_t *)I2S2_BASE, (i2sTxReg_t *)I2S3_BASE};

/**
 * @brief   get i2s tx channel register base address
 * @param   channel: i2s tx channel
 * @return 	I2S0_BASE / I2S1_BASE / I2S2_BASE / I2S3_BASE
 */
i2sTxReg_t *drv_l1_i2s_tx_get_register_base_addr(INT32U channel)
{
	switch (channel)
	{
		case 0:
			return (i2sTxReg_t *)I2S0_BASE;
		case 1:
			return (i2sTxReg_t *)I2S1_BASE;
		case 2:
			return (i2sTxReg_t *)I2S2_BASE;
		case 3:
			return (i2sTxReg_t *)I2S3_BASE;
		default:
			DBG_PRINT("illegal TX channel %u\r\n", channel);
			return (i2sTxReg_t *)I2S0_BASE;
	}
}

/**
 * @brief   clear i2s tx channel FIFO
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_clear_fifo(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL |= 0x040000;		// clear FIFO
    i2s->TX_CTRL &= (~0x040000);	// clear FIFO
}

/**
 * @brief   disable i2s tx channel
 * @param   channel: i2s tx channel
 * @param   wait: wait data get
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_disable(INT32U channel, INT8U wait)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    INT32U j;

    for (j = 0; j < 1000; j++)
    {
        if (j == 0)
          i2s->TX_CTRL &= (~0x1);			// Disable I2S_TX
        if (!wait || ((i2s->TX_CTRL & (1<<0)) == 0))
        {
            if (wait)
            {
              DBG_PRINT("TX disable require %d wait. TX_CTRL=0x%08x TX_Status=0x%08x\r\n", j, i2s->TX_CTRL, i2s->TX_STATUS);
            }
            return STATUS_OK;
        }
    }

    DBG_PRINT("TX disable timeout after %d wait. TX_CTRL=0x%08x TX_STATUS=0x%08x\r\n", j, i2s->TX_CTRL, i2s->TX_STATUS);

    return STATUS_FAIL;
}

/**
 * @brief   enable i2s tx channel
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_enable(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL |= 1;
}

/**
 * @brief   disable i2s tx channel half empty interrupt
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_disable_interrupt(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL &= (~(1<< K_I2S_TX_CTL_BIT_EN_IRT)); // disable interrupt

	vic_irq_disable(VIC_I2S_TX);
    #if C_I2S_TX_USE_FIQ == 1
    vic_fiq_disable(VIC_I2S_TX);
    #endif
	vic_irq_unregister(VIC_I2S_TX);
}

/**
 * @brief   enable i2s tx channel half empty interrupt
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_enable_interrupt(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    vic_irq_register(VIC_I2S_TX, I2STX_IRQHandler);
    vic_irq_enable(VIC_I2S_TX);
    #if C_I2S_TX_USE_FIQ == 1
	vic_fiq_enable(VIC_I2S_TX);
    #endif
    i2s->TX_CTRL |= (1<< K_I2S_TX_CTL_BIT_EN_IRT); // enable interrupt
}

 /**
 * @brief   i2s tx channel transfer dma data
 * @param   channel: i2s tx channel
 * @param   srcBuf: source buffer pointer
 * @param   BufLen: buffer length
 * @param   notify: notify pointer
 * @param   pchannel: i2s tx dma channel
 * @return 	status: >=0 is success, <0 is fail.
 */
INT32S drv_l1_i2s_tx_dma_transfer(INT32U channel, INT8U *srcBuf, INT32U BufLen, volatile INT8S *notify, INT8U *pchannel)
{
	volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    DMA_STRUCT dma_struct;
    INT32S status;

    if (notify != NULL)
        *notify = C_DMA_STATUS_WAITING;
    if (pchannel != NULL)
        *pchannel = 0xff;

    memset(&dma_struct, 0x00, sizeof(dma_struct));

    dma_struct.s_addr = (INT32U) srcBuf;
    dma_struct.t_addr = (INT32U) &(i2s->TX_DATA);
    dma_struct.width = DMA_DATA_WIDTH_4BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
    dma_struct.count = (INT32U) BufLen;
    dma_struct.notify = notify;
    dma_struct.timeout = 0;

    status = drv_l1_dma_transfer(&dma_struct);
    if ( status != 0)
      return status;
    else if (pchannel != NULL)
      *pchannel = dma_struct.channel;

    return STATUS_OK;
}

/**
 * @brief   init i2s tx channel
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_init(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    if (channel == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_TX1, 1);
    else if (channel == 2)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_TX2, 1);
    else if (channel == 3)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_TX3, 1);

    // since I2S TX fifo sram use clock source of TX0, when use TX1~3 also need to turn on TX0 clock source
    drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_TX0, 1);
    drv_l1_clock_set_peripheral_clock_enable(SRC_AUDIO_MCLK, 1);
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S, 1);

    R_EXTIO_EN |= (1L << ((channel << 1) + 1)); // assign gpio pins for I2S external codec mode

    i2s->TX_CTRL = 0x20104024;  // default
    i2s->TX_CTRL &= (~0x07000000);	// reset BCLK
    i2s->TX_CTRL |= 0x04000000;	// BCLK = MCLK / 4
}

/**
 * @brief   check i2s tx all channel idle status
 * @param   none
 * @return 	1: all channel idle, 0: channel is being used
 */
INT8U drv_l1_i2s_tx_check_is_all_idle(void)
{
    volatile i2sTxReg_t *i2s0 = drv_l1_i2s_tx_get_register_base_addr(0);
    volatile i2sTxReg_t *i2s1 = drv_l1_i2s_tx_get_register_base_addr(1);
    volatile i2sTxReg_t *i2s2 = drv_l1_i2s_tx_get_register_base_addr(2);
    volatile i2sTxReg_t *i2s3 = drv_l1_i2s_tx_get_register_base_addr(3);
    INT8U is_all_idle = 0;

    if (    ((i2s0->TX_CTRL & 0x1) == 0) && \
            ((i2s1->TX_CTRL & 0x1) == 0) && \
            ((i2s2->TX_CTRL & 0x1) == 0) && \
            ((i2s3->TX_CTRL & 0x1) == 0)    \
        )
        is_all_idle = 1;

    return is_all_idle;
}

/**
 * @brief   uninit i2s tx channel
 * @param   channel: i2s tx channel
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_uninit(INT32U channel)
{
    INT32S ret;
    INT8U is_all_idle;

    ret = drv_l1_i2s_tx_disable(channel, 1);

    R_EXTIO_EN &= ~(1L << ((channel << 1) + 1)); // release gpio pins from I2S purpose

    is_all_idle = drv_l1_i2s_tx_check_is_all_idle();
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S, 0);
    drv_l1_clock_set_peripheral_clock_enable(SRC_AUDIO_MCLK, 0);

    if (channel == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_TX1, 0);
    else if (channel == 2)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_TX2, 0);
    else if (channel == 3)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_TX3, 0);

    // to turn off TX0 clock source, have to ensure also TX1~3 is currently not enable
    if (is_all_idle)
        drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_TX0, 0);

    return ret;
}

/**
 * @brief   get i2s tx channel FIFO count
 * @param   channel: i2s tx channel
 * @return 	count
 */
INT32U drv_l1_i2s_tx_get_fifo_count(INT32U channel)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    INT32U left_fifo_count;

    left_fifo_count = ((i2s->TX_STATUS >> 3) & 0x3f);

    return left_fifo_count;
}

/**
 * @brief   set i2s tx channel sending mode
 * @param   channel: i2s tx channel
 * @param   mode  - 0:MSB sending first (Default), 1:LSB sending first
 * @return 	none
 */
void drv_l1_i2s_tx_set_bit_send_mode(INT32U channel, INT8U mode)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(1<<4))) | ((mode & 0x1) << 4);
}

/**
 * @brief   set i2s tx channel valid data length
 * @param   channel: i2s tx channel
 * @param   data_size: data size
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_set_data_bit_length(INT32U channel, INT32U data_size)
{
	volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    INT8U k, data_mode_total;
    INT8U *data_mode_list;

    data_mode_total = sizeof(i2s_tx_data_mode_list);
    data_mode_list = &i2s_tx_data_mode_list[0];

    for (k=0; k < data_mode_total; k++)
    {
        if (data_size == data_mode_list[k])
        {
            i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x7) << 6))) | (((INT32U)k) << 6);

            return STATUS_OK;
        }
    }

    return STATUS_FAIL;
}

/**
 * @brief   set i2s tx channel edge mode
 * @param   channel: i2s tx channel
 * @param   mode  - 0:serial clock falling edge (Default), 1:serial clock rising edge
 * @return 	none
 */
void drv_l1_i2s_tx_set_edge_mode(INT32U channel, INT8U mode)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(1<<3))) | ((mode & 0x1) << 3);
}

/**
 * @brief   set i2s tx channel first frame polarity
 * @param   channel: i2s tx channel
 * @param   polar  - 0:left frame (Default), 1:right frame
 * @return 	none
 */
void drv_l1_i2s_tx_set_first_frame_left_right(INT32U channel, INT8U polar)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(1<<1))) | ((polar & 0x1) << 1);
}

/**
 * @brief   set i2s tx channel frame polarity selection
 * @param   channel: i2s tx channel
 * @param   polar  - 0:LRCK =0 is right frame (Default), 1:LRCK =0 is left frame
 * @return 	none
 */
void drv_l1_i2s_tx_set_frame_polar(INT32U channel, INT8U polar)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(1<<2))) | ((polar & 0x1) << 2);
}

/**
 * @brief   set i2s tx channel frame size
 * @param   channel: i2s tx channel
 * @param   frame_size: frame size
 * @param   slave  - 0:master , 1:slave
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_set_frame_size(INT32U channel, INT32U frame_size, INT8U slave)
{
	volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    INT8U k, frame_size_total;
    INT8U *frame_size_list;

    if (slave)
    {
       frame_size_total = sizeof(i2s_tx_slave_frame_size_list);
       frame_size_list = &i2s_tx_slave_frame_size_list[0];
    }
    else
    {
       frame_size_total = sizeof(i2s_tx_master_frame_size_list);
       frame_size_list = &i2s_tx_master_frame_size_list[0];
    }
    for (k=0; k < frame_size_total; k++)
    {
        if (frame_size == frame_size_list[k])
        {
            i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0xf) << 28))) | (((INT32U)k) << 28);

            return STATUS_OK;
        }
    }

    return STATUS_FAIL;
}

/**
 * @brief   set i2s tx channel framing mode
 * @param   channel: i2s tx channel
 * @param   mode  - 0:I2S mode, 1:Normal Mode, 2:DSP mode, 3:DSP mode
 * @return 	none
 */
void drv_l1_i2s_tx_set_framing_mode(INT32U channel, INT8U mode)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x3) << 11))) | (((INT32U)mode) << 11);
}

/**
 * @brief   set i2s tx channel MCLK divider
 * @param   channel: i2s tx channel
 * @param   mclk_div: MCLK divider
 * @return 	none
 */
void drv_l1_i2s_tx_set_mclk_divider(INT32U channel, INT32U mclk_div)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x7) << 24))) | (((INT32U)mclk_div) << 24);
}

/**
 * @brief   set i2s tx channel merge mode
 * @param   channel: i2s tx channel
 * @param   merge  - 0:disable, 1:enable
 * @return 	none
 */
void drv_l1_i2s_tx_set_merge_mode(INT32U channel, INT32U merge)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x1) << 20))) | ((merge & 0x1) << 20);
}

/**
 * @brief   set i2s tx channel mono mode
 * @param   channel: i2s tx channel
 * @param   mono  - 0:it is stereo, receive both Left and Right channel, 1:received Left or Right channel according to FramePolarity and FirstFrameLR polarity
 * @return 	none
 */
void drv_l1_i2s_tx_set_mono(INT32U channel, INT32U mono)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x1) << 22))) | ((mono & 0x1) << 22);
}

/**
 * @brief   set i2s tx channel normal mode alignment
 * @param   channel: i2s tx channel
 * @param   mode  - 0:Transmitted data right aligned, 1:Transmitted data left aligned
 * @return 	none
 */
void drv_l1_i2s_tx_set_normal_mode_bit_align(INT32U channel, INT8U mode)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(1<<5))) | ((mode & 0x1) << 5);
}

// note that, valid only when MEGE is on
/**
 * @brief   set i2s tx right channel at LSB
 * @param   channel: i2s tx channel
 * @param   r_lsb  - 0:right channel data is at [31:16], left is at [15:0], 1:right channel data is at [15:0], left is at [31:16]
 * @return 	none
 */
void drv_l1_i2s_tx_set_right_channel_at_lsb_byte(INT32U channel, INT32U r_lsb)
{
    volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);

    i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x1) << 21))) | ((r_lsb & 0x1) << 21);
}

/**
 * @brief   set i2s tx channel SPU frequency samplerate
 * @param   channel: i2s tx channel
 * @param   spu_fs: SPU frequency samplerate
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_set_spu_fs(INT32U channel, INT32U spu_fs)
{
	volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    INT8U k, spu_fs_total;
    INT16U *spu_fs_list;

    spu_fs_total = sizeof(i2s_tx_spu_fs_list)/sizeof(INT16U);
    spu_fs_list = &i2s_tx_spu_fs_list[0];
    spu_fs = spu_fs/2 *2;

    for (k=0; k < spu_fs_total; k++)
    {
        if (spu_fs == spu_fs_list[k])
        {
            i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x3) << 9))) | ((k & 0x3) << 9);
            DBG_PRINT("spu use FS=%u\r\n", spu_fs);

            return STATUS_OK;
        }
    }
    DBG_PRINT("Serious Error: Cant find suitable spu FS=%u\r\n", spu_fs);
    for (k=0; k < spu_fs_total; k++)
    {
        if (spu_fs <= spu_fs_list[k])
        {
            i2s->TX_CTRL = (i2s->TX_CTRL & (~(((INT32U)0x3) << 9))) | ((k & 0x3) << 9);
            DBG_PRINT("spu use FS=%u, near=%u\r\n", spu_fs, spu_fs_list[k]);

            return STATUS_OK;
        }
    }

    return STATUS_FAIL;
}

INT32S drv_l1_i2s_tx_resgiter_isr(INT32U channel, void (*isr_func)(INT32U channel, INT32U isr_global), INT32U isr_global)
{
    tx_isr[channel].isr_func = isr_func;
    tx_isr[channel].isr_global = isr_global;

    return 0;
}

void I2STX_IRQHandler(void)
{
    INT8U k;
    INT8U idx;

	for (k = 0; k < 4; k++)
	{
        idx = tx_isr_round_start + k;
        if (idx >= 4)
            idx = idx - 4;
        if ((tx_i2s_regs_list[idx]->TX_CTRL & (1<< K_I2S_TX_CTL_BIT_IRT_FLAG)) != 0)
        {
            if (tx_isr[idx].isr_func != 0)
                tx_isr[idx].isr_func(idx, tx_isr[idx].isr_global);
        }
    } // end for
    tx_isr_round_start++;
    if (tx_isr_round_start >= 4)
        tx_isr_round_start = 0;
}

/**
 * @brief   i2s tx dma double buffer put
 * @param   channel: i2s tx channel
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @param   os_q: os queue
 * @param   notify: pointer to 1 byte notify variable
 * @param   isr_func a callback function to be called when dma done
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_dbf_put(INT32U channel, INT16S *data, INT32U cwlen, osMessageQId os_q, volatile INT8S *notify, void (*isr_func)(INT8U, INT8U))
{
	INT32S status;
	volatile i2sTxReg_t *i2s = drv_l1_i2s_tx_get_register_base_addr(channel);
    DMA_STRUCT *pi2s_tx_dma_dbf = &i2s_tx_dma_dbf[channel];

    if (notify != NULL)
        *notify = C_DMA_STATUS_WAITING;
	memset(pi2s_tx_dma_dbf, 0x00, sizeof(DMA_STRUCT));
	pi2s_tx_dma_dbf->s_addr = (INT32U) data;
	pi2s_tx_dma_dbf->t_addr = (INT32U) &(i2s->TX_DATA);
	if (cwlen & 0x1)
	{
        pi2s_tx_dma_dbf->width = DMA_DATA_WIDTH_2BYTE;
        pi2s_tx_dma_dbf->count = (INT32U)cwlen;
	}
	else
	{
        pi2s_tx_dma_dbf->width = DMA_DATA_WIDTH_4BYTE;
        pi2s_tx_dma_dbf->count = (INT32U)(cwlen >> 1);
    }
	pi2s_tx_dma_dbf->notify = notify;
	pi2s_tx_dma_dbf->timeout = 0;

	status = drv_l1_dma_transfer_double_buf_with_callback(pi2s_tx_dma_dbf, os_q, isr_func);
	if(status < 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   i2s tx dma double buffer set
 * @param   channel: i2s tx channel
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_dbf_set(INT32U channel, INT16S *data, INT32U cwlen)
{
	INT32S status;
    DMA_STRUCT *pi2s_tx_dma_dbf = &i2s_tx_dma_dbf[channel];

	pi2s_tx_dma_dbf->s_addr = (INT32U) data;

	if (pi2s_tx_dma_dbf->width == DMA_DATA_WIDTH_4BYTE)
        pi2s_tx_dma_dbf->count = (INT32U)(cwlen>>1);
	else
        pi2s_tx_dma_dbf->count = cwlen;

	status = drv_l1_dma_transfer_double_buf_set(pi2s_tx_dma_dbf);
	if(status < 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   i2s free tx dma channel
 * @param   channel: i2s tx channel
 * @return 	none
 */
void drv_l1_i2s_tx_dbf_free(INT32U channel)
{
    DMA_STRUCT *pi2s_tx_dma_dbf = &i2s_tx_dma_dbf[channel];

	drv_l1_dma_transfer_double_buf_free(pi2s_tx_dma_dbf);
	pi2s_tx_dma_dbf->channel = 0xff;
}

/**
 * @brief   i2s get tx dma double buffer status
 * @param   channel: i2s tx channel
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_tx_dbf_status_get(INT32U channel)
{
	DMA_STRUCT *pi2s_tx_dma_dbf = &i2s_tx_dma_dbf[channel];

	if (pi2s_tx_dma_dbf->channel == 0xff) {
		return -1;
	}

	return drv_l1_dma_dbf_status_get(pi2s_tx_dma_dbf->channel);
}

/**
 * @brief   i2s get tx dma status
 * @param   channel: i2s tx channel
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_tx_dma_status_get(INT32U channel)
{
	DMA_STRUCT *pi2s_tx_dma_dbf = &i2s_tx_dma_dbf[channel];

	if (pi2s_tx_dma_dbf->channel == 0xff) {
		return -1;
	}

	return drv_l1_dma_status_get(pi2s_tx_dma_dbf->channel);
}
