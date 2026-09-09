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
#define C_I2S_RX_USE_FIQ             0 // set this to 1 to use I2S interrupt in fast interrupt modde

/* Rx Control Register */
#define I2S_RX_EN					(1 << 0)
#define I2S_RX_FIRST_FRAME_R		(1 << 1)
#define I2S_RX_FRAME_POLARITY_L		(1 << 2)
#define I2S_RX_RISING_MODE			(1 << 3)
#define I2S_RX_LSB_MODE				(1 << 4)
#define I2S_RX_LEFT_ALIGN			(1 << 5)

#define I2S_RX_VALIDDATA_16BIT		(0x00 << 6)
#define I2S_RX_VALIDDATA_18BIT		(0x01 << 6)
#define I2S_RX_VALIDDATA_20BIT		(0x02 << 6)
#define I2S_RX_VALIDDATA_22BIT		(0x03 << 6)
#define I2S_RX_VALIDDATA_24BIT		(0x04 << 6)
#define I2S_RX_VALIDDATA_32BIT		(0x05 << 6)
#define I2S_RX_VALIDDATA_08BIT		(0x06 << 6)
#define I2S_RX_VALIDDATA_MASK		(0x07 << 6)

#define I2S_RX_FRAMESIZE_16BIT		(0x00 << 9)
#define I2S_RX_FRAMESIZE_24BIT		(0x01 << 9)
#define I2S_RX_FRAMESIZE_32BIT		(0x02 << 9)
#define I2S_RX_FRAMESIZE_AUTO		(0x03 << 9)
#define I2S_RX_FRAMESIZE_MASK		(0x03 << 9)

#define I2S_RX_MODE_I2S				(0x00 << 11)
#define I2S_RX_MODE_NORMAL			(0x01 << 11)
#define I2S_RX_MODE_DSP				(0x02 << 11)
#define I2S_RX_MODE_MASK			(0x03 << 11)

#define I2S_RX_MASTER_MODE			(1 << 13)
#define I2S_RX_IRQ_POLARITY_HI		(1 << 14)
#define I2S_RX_OVWR_CLEAR			(1 << 15)
#define I2S_RX_INT_EN				(1 << 16)
#define I2S_RX_IRQ_FLAG				(1 << 17)
#define I2S_RX_FIFO_CLEAR			(1 << 18)
#define I2S_RX_MERGE				(1 << 19)
#define I2S_RX_MERGE_R_LSB			(1 << 20)
#define I2S_RX_MONO_MODE			(1 << 21)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		gp_memset((INT8S *)ptr, 0x00, cblen)

#define APPL_ENABLE				(R_SYSTEM_CKGEN_CTRL |= 0x10)
#define APPL_DISABLE			(R_SYSTEM_CKGEN_CTRL &= ~(0x10))

#define MCLK_PLL_67MHz 			(R_SYSTEM_APLL_SEL |= 0x1)
#define MCLK_PLL_73MHz 			(R_SYSTEM_APLL_SEL &= ~(0x1))

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
void I2SRX_IRQHandler(void);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct i2sRxIsr_s
{
    void (*isr_func)(INT32U channel, INT32U isr_global);
    INT32U isr_global;
} i2sRxIsr_t;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static DMA_STRUCT i2s_rx_dma_dbf[4] = {{0},{0},{0},{0}};
static INT8U i2s_rx_data_mode_list[] = {16, 18, 20, 22, 24, 32, 32, 32};
static INT8U i2s_rx_frame_size_list[] = {16, 24, 32, 0};
static i2sRxIsr_t rx_isr[4]={{0,0}, {0,0}, {0,0}, {0,0}};
static INT8U rx_isr_round_start = 0;
static volatile i2sRxReg_t *rx_i2s_regs_list[4] = {(i2sRxReg_t *)I2S0_BASE, (i2sRxReg_t *)I2S1_BASE, (i2sRxReg_t *)I2S2_BASE, (i2sRxReg_t *)I2S3_BASE};

/**
 * @brief   get i2s rx channel register base address
 * @param   channel: i2s rx channel
 * @return 	I2S0_BASE / I2S1_BASE / I2S2_BASE / I2S3_BASE
 */
i2sRxReg_t *drv_l1_i2s_rx_get_register_base_addr(INT32U channel)
{
	switch (channel)
	{
		case 0:
			return (i2sRxReg_t *)I2S0_BASE;
		case 1:
			return (i2sRxReg_t *)I2S1_BASE;
		case 2:
			return (i2sRxReg_t *)I2S2_BASE;
		case 3:
			return (i2sRxReg_t *)I2S3_BASE;
		default:
			DBG_PRINT("illegal RX channel %u\r\n", channel);
			return (i2sRxReg_t *)I2S0_BASE;
	}
}

/**
 * @brief   clear i2s rx channel FIFO
 * @param   channel: i2s rx channel
 * @return 	none
 */
void drv_l1_i2s_rx_clear_fifo(INT32U channel)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL |= 0x040000;           // clear FIFO
    i2s->RX_CTRL &= (~0x040000);        // clear FIFO
}

/**
 * @brief   disable i2s rx channel
 * @param   channel: i2s rx channel
 * @param   wait: wait data get
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_disable(INT32U channel, INT8U wait)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    INT32U j;

    for (j = 0; j < 1000; j++)
    {
        if (j == 0)
          i2s->RX_CTRL &= (~0x1);			// Disable I2S_RX
        if (!wait || ((i2s->RX_CTRL & 0x1) == 0))
        {
            if (wait)
            {
                DBG_PRINT("RX disable require %d wait. RX_CTRL=0x%08x RX_STATUS=0x%08x\r\n", j, i2s->RX_CTRL, i2s->RX_STATUS);
            }
            return STATUS_OK;
        }
    }
    DBG_PRINT("RX disable timeout after %d wait. RX_CTRL=0x%08x RX_STATUS=0x%08x\r\n", j, i2s->RX_CTRL, i2s->RX_STATUS);

    return STATUS_FAIL;
}

/**
 * @brief   enable i2s rx channel
 * @param   channel: i2s rx channel
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_enable(INT32U channel, INT8U wait)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    INT32U j;

    for (j = 0; j < 1000; j++)
    {
        if (j == 0)
          i2s->RX_CTRL |= 0x1;			// Enable I2S_RX
        if (!wait || ((i2s->RX_CTRL & 0x1) == 0x1))
        {
            if (wait)
            {
                DBG_PRINT("RX enable require %d wait. RX_CTRL=0x%08x RX_STATUS=0x%08x\r\n", j, i2s->RX_CTRL, i2s->RX_STATUS);
            }
            return STATUS_OK;
        }
    }
    DBG_PRINT("RX enable timeout after %d wait. RX_CTRL=0x%08x RX_STATUS=0x%08x\r\n", j, i2s->RX_CTRL, i2s->RX_STATUS);

    return STATUS_FAIL;
}

/**
 * @brief   disable i2s rx channel half empty interrupt
 * @param   channel: i2s rx channel
 * @return 	none
 */
void drv_l1_i2s_rx_disable_interrupt(INT32U channel)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL &= (~(1<< K_I2S_RX_CTL_BIT_EN_IRT)); // disable interrupt
    vic_irq_disable(VIC_I2S_RX);
    #if C_I2S_RX_USE_FIQ == 1
    vic_fiq_disable(VIC_I2S_RX);
    #endif
    vic_irq_unregister(VIC_I2S_RX);
}

/**
 * @brief   enable i2s rx channel half empty interrupt
 * @param   channel: i2s rx channel
 * @return 	none
 */
void drv_l1_i2s_rx_enable_interrupt(INT32U channel)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    vic_irq_register(VIC_I2S_RX, I2SRX_IRQHandler);
    vic_irq_enable(VIC_I2S_RX);
    #if C_I2S_RX_USE_FIQ == 1
    vic_fiq_enable(VIC_I2S_RX);
    #endif
    i2s->RX_CTRL |= (1<< K_I2S_RX_CTL_BIT_EN_IRT); // enable interrupt
}

 /**
 * @brief   i2s rx channel transfer dma data
 * @param   channel: i2s rx channel
 * @param   dstBuf: destination buffer pointer
 * @param   BufLen: buffer length
 * @param   notify: notify pointer
 * @return 	status: >=0 is success, <0 is fail.
 */
INT32S drv_l1_i2s_rx_dma_transfer(INT32U channel,  INT8U* dstBuf, INT32U BufLen, volatile INT8S *notify)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    DMA_STRUCT dma_struct = {0};
    INT32S status;

    *notify = C_DMA_STATUS_WAITING;

    dma_struct.s_addr = (INT32U) &(i2s->RX_DATA);
    dma_struct.t_addr = (INT32U) dstBuf;
    dma_struct.width = DMA_DATA_WIDTH_4BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
    dma_struct.count = (INT32U) BufLen;
    dma_struct.notify = notify;
    dma_struct.timeout = 0;

    status = drv_l1_dma_transfer(&dma_struct);

    if ( status != 0)
      return status;

    return STATUS_OK;
}

/**
 * @brief   init i2s rx channel
 * @param   channel: i2s rx channel
 * @return 	none
 */
void drv_l1_i2s_rx_init(INT32U channel)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    if (channel == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX1, 1);
    else if (channel == 2)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX2, 1);
    else if (channel == 3)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX3, 1);

    // since I2S RX fifo sram use clock source of RX0, when use RX1~3 also need to turn on RX0 clock source
    drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_RX0, 1);
    drv_l1_clock_set_peripheral_clock_enable(SRC_AUDIO_MCLK, 1);
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S, 1);
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S_RX, 1);

    R_EXTIO_EN |= (1L << ((channel << 1))); // assign gpio pins for I2S external codec mode

    // [19]=1 MERGE, [13]=0 SLVMOVE set RX to slave mode, [10:9]=10 Frame size 32 bit, [8:6]=000 16 bit, [2]=1 LRCK=1. LRCK=0 is left frame
    i2s->RX_CTRL = 0x84424;
}

/**
 * @brief   check i2s rx all channel idle status
 * @param   none
 * @return 	1: all channel idle, 0: channel is being used
 */
INT8U drv_l1_i2s_rx_check_is_all_idle(void)
{
    volatile i2sRxReg_t *i2s0 = drv_l1_i2s_rx_get_register_base_addr(0);
    volatile i2sRxReg_t *i2s1 = drv_l1_i2s_rx_get_register_base_addr(1);
    volatile i2sRxReg_t *i2s2 = drv_l1_i2s_rx_get_register_base_addr(2);
    volatile i2sRxReg_t *i2s3 = drv_l1_i2s_rx_get_register_base_addr(3);
    INT8U is_all_idle = 0;

    if (    ((i2s0->RX_CTRL & 0x1) == 0) && \
            ((i2s1->RX_CTRL & 0x1) == 0) && \
            ((i2s2->RX_CTRL & 0x1) == 0) && \
            ((i2s3->RX_CTRL & 0x1) == 0)    \
        )
        is_all_idle = 1;

    return is_all_idle;
}

/**
 * @brief   uninit i2s rx channel
 * @param   channel: i2s rx channel
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_uninit(INT32U channel)
{
    INT32S ret;
    INT8U is_all_idle;

    ret = drv_l1_i2s_rx_disable(channel, 1);

    R_EXTIO_EN &= ~(1L << ((channel << 1))); // release gpio pins from I2S purpose

    is_all_idle = drv_l1_i2s_rx_check_is_all_idle();
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S_RX, 0);
    drv_l1_clock_set_peripheral_clock_enable(SRC_I2S, 0);
    drv_l1_clock_set_peripheral_clock_enable(SRC_AUDIO_MCLK, 0);

    if (channel == 1)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX1, 0);
    else if (channel == 2)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX2, 0);
    else if (channel == 3)
        drv_l1_clock_set_system_clk_en(CLK_EN3_I2S_RX3, 0);

    // to turn off RX0 clock source, have to ensure also RX1~3 is currently not enable
    if (is_all_idle)
        drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_RX0, 0);

    return ret;
}

/**
 * @brief   get i2s rx channel FIFO count
 * @param   channel: i2s rx channel
 * @return 	count
 */
INT32U drv_l1_i2s_rx_get_fifo_count(INT32U channel)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    INT32U fifo_count;

    fifo_count = ((i2s->RX_STATUS >> 2) & 0x3f);

    return fifo_count;
}

/**
 * @brief   set i2s rx channel receiving mode
 * @param   channel: i2s rx channel
 * @param   mode  - 0:MSB receiving first (Default), 1:LSB receiving first
 * @return 	none
 */
void drv_l1_i2s_rx_set_bit_receive_mode(INT32U channel, INT8U mode)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(1<<4))) | ((mode & 0x1) << 4);
}

/**
 * @brief   set i2s rx channel valid data length
 * @param   channel: i2s rx channel
 * @param   data_size: data size
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_set_data_bit_length(INT32U channel, INT32U data_size)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    INT8U k, data_mode_total;
    INT8U *data_mode_list;

    data_mode_total = sizeof(i2s_rx_data_mode_list);
    data_mode_list = &i2s_rx_data_mode_list[0];

    for (k=0; k < data_mode_total; k++)
    {
        if (data_size == data_mode_list[k])
        {
            i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x7) << 6))) | (((INT32U)k) << 6);

            return STATUS_OK;
        }
    }

    return STATUS_FAIL;
}

/**
 * @brief   set i2s rx channel edge mode
 * @param   channel: i2s rx channel
 * @param   mode  - 0:serial clock falling edge (Default), 1:serial clock rising edge
 * @return 	none
 */
void drv_l1_i2s_rx_set_edge_mode(INT32U channel, INT8U mode)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(1<<3))) | ((mode & 0x1) << 3);
}

/**
 * @brief   set i2s rx channel first frame polarity
 * @param   channel: i2s rx channel
 * @param   polar  - 0:left frame (Default), 1:right frame
 * @return 	none
 */
void drv_l1_i2s_rx_set_first_frame_left_right(INT32U channel, INT8U polar)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(1<<1))) | ((polar & 0x1) << 1);
}

/**
 * @brief   set i2s rx channel frame polarity selection
 * @param   channel: i2s rx channel
 * @param   polar  - 0:LRCK =0 is right frame (Default), 1:LRCK =0 is left frame
 * @return 	none
 */
void drv_l1_i2s_rx_set_frame_polar(INT32U channel, INT8U polar)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(1<<2))) | ((polar & 0x1) << 2);
}

/**
 * @brief   set i2s rx channel frame size
 * @param   channel: i2s rx channel
 * @param   frame_size: frame size
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_set_frame_size(INT32U channel, INT32U frame_size)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
    INT8U k, frame_size_total;
    INT8U *frame_size_list;

    frame_size_total = sizeof(i2s_rx_frame_size_list);
    frame_size_list = &i2s_rx_frame_size_list[0];

    for (k=0; k < frame_size_total; k++)
    {
        if (frame_size == frame_size_list[k])
        {
            i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x3) << 9))) | (((INT32U)k) << 9);

            return STATUS_OK;
        }
    }

    return STATUS_FAIL;
}

/**
 * @brief   set i2s rx channel framing mode
 * @param   channel: i2s rx channel
 * @param   mode  - 0:I2S mode, 1:Normal Mode, 2:DSP mode, 3:DSP mode
 * @return 	none
 */
void drv_l1_i2s_rx_set_framing_mode(INT32U channel, INT8U mode)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x3) << 11))) | (((INT32U)mode) << 11);
}

/**
 * @brief   set i2s rx channel merge mode
 * @param   channel: i2s rx channel
 * @param   merge  - 0:disable, 1:enable
 * @return 	none
 */
void drv_l1_i2s_rx_set_merge_mode(INT32U channel, INT32U merge)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x1) << 19))) | ((merge & 0x1) << 19);
}

/**
 * @brief   set i2s rx channel mono mode
 * @param   channel: i2s rx channel
 * @param   mono  - 0:it is stereo, receive both Left and Right channel, 1:received Left or Right channel according to FramePolarity and FirstFrameLR polarity
 * @return 	none
 */
void drv_l1_i2s_rx_set_mono(INT32U channel, INT32U mono)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x1) << 21))) | ((mono & 0x1) << 21);
}

/**
 * @brief   set i2s rx channel normal mode alignment
 * @param   channel: i2s rx channel
 * @param   mode  - 0:Recieved data right aligned, 1:Recieved data left aligned
 * @return 	none
 */
void drv_l1_i2s_rx_set_normal_mode_bit_align(INT32U channel, INT8U mode)
{
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(1<<5))) | ((mode & 0x1) << 5);
}

// note that, valid only when MEGE is on
/**
 * @brief   set i2s rx right channel at LSB
 * @param   channel: i2s rx channel
 * @param   r_lsb  - 0:right channel data is at [31:16], left is at [15:0], 1:right channel data is at [15:0], left is at [31:16]
 * @return 	none
 */
void drv_l1_i2s_rx_set_right_channel_at_lsb_byte(INT32U channel, INT32U r_lsb)
{
    volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);

    i2s->RX_CTRL = (i2s->RX_CTRL & (~(((INT32U)0x1) << 20))) | ((r_lsb & 0x1) << 20);
}

INT32S drv_l1_i2s_rx_resgiter_isr(INT32U channel, void (*isr_func)(INT32U channel, INT32U isr_global), INT32U isr_global)
{
    rx_isr[channel].isr_func = isr_func;
    rx_isr[channel].isr_global = isr_global;

    return 0;
}

void I2SRX_IRQHandler(void)
{
    INT8U k;
    INT8U idx;

	for (k = 0; k < 4; k++)
	{
        idx = rx_isr_round_start + k;
        if (idx >= 4)
            idx = idx - 4;
        if ((rx_i2s_regs_list[idx]->RX_CTRL & (1<< K_I2S_RX_CTL_BIT_IRT_PEND)) != 0)
        {
            if (rx_isr[idx].isr_func != 0)
                rx_isr[idx].isr_func(idx, rx_isr[idx].isr_global);
        }
    } // end for
    rx_isr_round_start++;
    if (rx_isr_round_start >= 4)
        rx_isr_round_start = 0;
}

/**
 * @brief   i2s rx dma double buffer put
 * @param   channel: i2s rx channel
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @param   os_q: os queue
 * @param   notify: pointer to 1 byte notify variable
 * @param   isr_func a callback function to be called when dma done
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_dbf_put(INT32U channel, INT16S *data, INT32U cwlen, osMessageQId os_q, volatile INT8S *notify, void (*isr_func)(INT8U, INT8U))
{
	INT32S status;
	volatile i2sRxReg_t *i2s = drv_l1_i2s_rx_get_register_base_addr(channel);
	DMA_STRUCT *pi2s_rx_dma_dbf = &i2s_rx_dma_dbf[channel];

	 if (notify != NULL)
        *notify = C_DMA_STATUS_WAITING;
	memset(pi2s_rx_dma_dbf, 0x00, sizeof(DMA_STRUCT));
	pi2s_rx_dma_dbf->s_addr = (INT32U) &(i2s->RX_DATA);
	pi2s_rx_dma_dbf->t_addr = (INT32U) data;
	if (cwlen & 0x1)
	{
        pi2s_rx_dma_dbf->width = DMA_DATA_WIDTH_2BYTE;
        pi2s_rx_dma_dbf->count = (INT32U)cwlen;
	}
	else
	{
        pi2s_rx_dma_dbf->width = DMA_DATA_WIDTH_4BYTE;
        pi2s_rx_dma_dbf->count = (INT32U)(cwlen >> 1);
    }
	pi2s_rx_dma_dbf->notify = notify;
	pi2s_rx_dma_dbf->timeout = 0;

	status = drv_l1_dma_transfer_double_buf_with_callback(pi2s_rx_dma_dbf, os_q, isr_func);
	if(status < 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   i2s rx dma double buffer set
 * @param   channel: i2s rx channel
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_dbf_set(INT32U channel, INT16S *data, INT32U cwlen)
{
	INT32S status;
	DMA_STRUCT *pi2s_rx_dma_dbf = &i2s_rx_dma_dbf[channel];

	pi2s_rx_dma_dbf->t_addr = (INT32U) data;

	if (pi2s_rx_dma_dbf->width == DMA_DATA_WIDTH_4BYTE)
        pi2s_rx_dma_dbf->count = (INT32U)(cwlen>>1);
	else
        pi2s_rx_dma_dbf->count = cwlen;

	status = drv_l1_dma_transfer_double_buf_set(pi2s_rx_dma_dbf);
	if(status < 0) {
		return status;
	}

	return STATUS_OK;
}

/**
 * @brief   i2s free rx dma channel
 * @param   channel: i2s rx channel
 * @return 	none
 */
void drv_l1_i2s_rx_dbf_free(INT32U channel)
{
	DMA_STRUCT *pi2s_rx_dma_dbf = &i2s_rx_dma_dbf[channel];

	drv_l1_dma_transfer_double_buf_free(pi2s_rx_dma_dbf);
	pi2s_rx_dma_dbf->channel = 0xff;
}

/**
 * @brief   i2s get rx dma double buffer status
 * @param   channel: i2s rx channel
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_dbf_status_get(INT32U channel)
{
	DMA_STRUCT *pi2s_rx_dma_dbf = &i2s_rx_dma_dbf[channel];

	if (pi2s_rx_dma_dbf->channel == 0xff) {
		return -1;
	}

	return drv_l1_dma_dbf_status_get(pi2s_rx_dma_dbf->channel);
}

/**
 * @brief   i2s get rx dma status
 * @param   channel: i2s rx channel
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_dma_status_get(INT32U channel)
{
	DMA_STRUCT *pi2s_rx_dma_dbf = &i2s_rx_dma_dbf[channel];

	if (pi2s_rx_dma_dbf->channel == 0xff) {
		return -1;
	}

	return drv_l1_dma_status_get(pi2s_rx_dma_dbf->channel);
}

/**************************************************************************
 *                   R X     A P I    FOR    AUDIO-IN                     *
 **************************************************************************/
/**
 * @brief   i2s rx form mic or line in LR path
 * @param   path: MIC_INPUT or LINE_IN_LR
 * @return 	none
 */
void drv_l1_i2s_rx_adc_set_input_path(I2S_RX_INPUT_PATH path)
{
    //move here setting to drv_l1_adc_mic_enable()
}

/**
 * @brief   i2s rx initalize
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_adc_init(void)
{
	drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_RX0, ENABLE);
	// APLL EN  (audio PLL)
	APPL_ENABLE;

	// set i2s for mic or line in
	R_I2SRX_CTRL |= I2S_RX_IRQ_FLAG | I2S_RX_OVWR_CLEAR;
	R_I2SRX_CTRL = 0x4820;					// Reset Value
	R_I2SRX_CTRL &= ~I2S_RX_MODE_MASK;		// I2S Mode
	R_I2SRX_CTRL |= I2S_RX_MODE_NORMAL;		// must Normal Mode because of front end design.
	R_I2SRX_CTRL |= I2S_RX_MERGE;  			// MERGE Mode
}

/**
 * @brief   i2s rx exit
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_adc_exit(void)
{
	R_I2SRX_CTRL = 0x4820;		// Reset Value

	// APLL disable
	APPL_DISABLE;

	// turn off MIC
	//move here setting to drv_l1_adc_mic_disable()

	drv_l1_clock_set_system_clk_en(CLK_EN2_I2S_RX0, DISABLE);
}

/**
 * @brief   i2s rx clear fifo
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_adc_fifo_clear(void)
{
    drv_l1_i2s_rx_clear_fifo(0);
}

/**
 * @brief   i2s rx set mono channel
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_adc_mono_ch_set(void)
{
	drv_l1_i2s_rx_set_mono(0, 1);

	return STATUS_OK;
}

/**
 * @brief   i2s rx set start
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_adc_start(void)
{
	drv_l1_i2s_rx_clear_fifo(0);
    drv_l1_i2s_rx_enable(0, 0);

	return STATUS_OK;
}

/**
 * @brief   i2s rx set stop
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_adc_stop(void)
{
    drv_l1_i2s_rx_disable(0, 0);
	drv_l1_i2s_rx_clear_fifo(0);

	return STATUS_OK;
}

/**
 * @brief   i2s rx dma double buffer put
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @param   os_q: os queue
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_adc_dbf_put(INT16S *data, INT32U cwlen, osMessageQId os_q)
{
	return drv_l1_i2s_rx_dbf_put(0, data, cwlen, os_q, NULL, NULL);
}

/**
 * @brief   i2s rx dma double buffer set
 * @param   data: buffer
 * @param   cwlen: length in short unit
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_adc_dbf_set(INT16S *data, INT32U cwlen)
{
	return drv_l1_i2s_rx_dbf_set(0, data, cwlen);
}

/**
 * @brief   i2s get rx dma double buffer status
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_adc_dbf_status_get(void)
{
	return drv_l1_i2s_rx_dbf_status_get(0);
}

/**
 * @brief   i2s get rx dma status
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_adc_dma_status_get(void)
{
	return drv_l1_i2s_rx_dma_status_get(0);
}

/**
 * @brief   i2s free rx dma channel
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_adc_dbf_free(void)
{
	drv_l1_i2s_rx_dbf_free(0);
}

/**
 * @brief   i2s rx set sample rate
 * @param   hz: sample rate
 * @return 	none
 */
void drv_l1_i2s_rx_adc_sample_rate_set(INT32U hz)
{
    INT32U freq_i2s_clk;
    freq_i2s_clk = hz * 256; //here need check real HW divider, sample rate isn't correct
    switch(hz)
    {
        case 8000:
        case 16000:
        case 24000:
        case 32000:
        case 48000:
            drv_l1_clock_set_audio_pll_freq(1, ENABLE);
        break;
        case 11025:
        case 22050:
        case 44100:
        case 88200:
            drv_l1_clock_set_audio_pll_freq(0, ENABLE);
        break;
        default:
            DBG_PRINT("not supported sample rate\r\n");
        break;
    }
    drv_l1_clock_set_peripheral_clock_freq(SRC_AUDIO_MCLK, freq_i2s_clk);
}
