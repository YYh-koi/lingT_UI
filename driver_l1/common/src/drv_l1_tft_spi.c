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
#include "drv_l2.h"

#define TFT_SPI0_CLOCK_SOURCE               CLK_EN4_TFT_SPI0//CLK_EN0_TFT_SPI CLK_EN4_TFT_SPI1
#define TFT_SPI1_CLOCK_SOURCE               CLK_EN4_TFT_SPI1//CLK_EN0_TFT_SPI
#define PPU_CLOCK_SOURCE                    CLK_EN1_PPU_FB_AHBMUX
#define FRAME_BUFFER_CLOCK_SOURCE           CLK_EN1_PPU_FRAME_BUF//CLK_EN1_DISPLAY_FRAME_BUFFER
#define DISPLAY_REG_CLOCK_SOURCE            CLK_EN1_PPU_REG_BANK//CLK_EN1_DISPAY_OUT_REG

#define TFT_SPI_SYSTEM_OS_ENABLE            1
#define TFT_SPI_CLOCK_ENABLE                1
#define TFT_SPI_CLOCK_DISABLE               0
#define C_TFT_SPI_QUEUE_SIZE		        5

#define TFT_SPI_FIQ_EN                      0
#define TFT_SPI1_IRQ_NUM                    VIC_TFT1_SPI//VIC_TFT1_SPI//VIC_TFT_SPI
#define TFT_SPI2_IRQ_NUM                    VIC_TFT2_SPI//VIC_TFT1_SPI//VIC_TFT_SPI


typedef struct
{												// Offset
    volatile INT32U	TFT_SPI_CTRL;				// 0x0000
    volatile INT32U	TFT_SPI_STS;			    // 0x0004
	volatile INT32U	TFT_SPI_TX_DAT;			    // 0x0008
	volatile INT32U	TFT_SPI_RX_DAT;			    // 0x000C
	volatile INT32U	TFT_SPI_CTRL2;				// 0x0010
	volatile INT32U	TFT_SPI_CTRL3;				// 0x0014
	volatile INT32U	TFT_SPI_VSYNC;				// 0x0018
	volatile INT32U	TFT_SPI_VDISP;				// 0x001C
	volatile INT32U	TFT_SPI_HSYNC;			    // 0x0020
	volatile INT32U	TFT_SPI_HDISP;		        // 0x0024
	volatile INT32U	TFT_SPI_DATA_CNT;		    // 0x0028
	volatile INT32U	TFT_SPI_TE_CNT;			    // 0x002C
}TFT_SPI_DEVICE_SFR;

typedef struct TFT_SPIDev_s
{
	INT8U init_done;
	INT8U started_flag;
	INT32U device_update_flag;
	INT32U device_frame_buf;
	volatile INT32U interrupt_flag;

	osMessageQId message_queue;
	osSemaphoreId Dev_hw_sem;
} TFT_SPIDev_t;

TFT_ISR spitft_isr_vblank_callback;
static INT32U tft_spi_dev_init_flag[TFT_SPI_NUM] = {0};
static TFT_SPIDev_t TFT_SPI_dev[TFT_SPI_NUM];
static void (*tft_spi_isr_callback)(INT8U tft_spi_dev, INT32U tft_spi_event);
static void (*display_isr_callback)(INT8U tft_spi_dev, INT32U tft_spi_event);

static __inline TFT_SPI_DEVICE_SFR *get_tft_spi_dev_reg_base(TFT_SPI_DEV_ID tft_spi_dev)
{
	if(tft_spi_dev == TFT_SPI1)
	{
		return (TFT_SPI_DEVICE_SFR *)TFT_SPI1_BASE;
	}
	else
	{
		return (TFT_SPI_DEVICE_SFR *)TFT_SPI0_BASE;
	}
}

/**
 * @brief   Scaler device get
 * @param   scaler_num[in]: SCALER_NUM
 * @return 	none
 */
static __inline TFT_SPIDev_t *get_tft_spi_device(TFT_SPI_DEV_ID scaler_num)
{
	if (scaler_num == TFT_SPI1)
		return &TFT_SPI_dev[TFT_SPI1];
    else
		return &TFT_SPI_dev[TFT_SPI0];
}

static INT16S drv_l1_tft_spi_clock_set(TFT_SPI_DEV_ID tft_spi_dev, INT16U enable)
{
    INT16S ret,value = 0;

    ret = drv_l1_clock_set_system_clk_en(DISPLAY_REG_CLOCK_SOURCE, enable);
    if(ret)
        value = ret;
    ret = drv_l1_clock_set_system_clk_en(FRAME_BUFFER_CLOCK_SOURCE, enable);
    if(ret)
        value = ret;
    ret = drv_l1_clock_set_system_clk_en(PPU_CLOCK_SOURCE, enable);
    if(ret)
        value = ret;
    if(tft_spi_dev == TFT_SPI1)
        ret = drv_l1_clock_set_system_clk_en(TFT_SPI1_CLOCK_SOURCE, enable);
    else
        ret = drv_l1_clock_set_system_clk_en(TFT_SPI0_CLOCK_SOURCE, enable);
    if(ret)
        value = ret;

    return value;
}

void drv_l1_tft_spi_lock(INT8U tft_spi_dev)
{
   TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);

#if TFT_SPI_SYSTEM_OS_ENABLE == 1
    if(pTFT_SPIDev->Dev_hw_sem)
        osSemaphoreWait(pTFT_SPIDev->Dev_hw_sem, osWaitForever);
#endif
}

void drv_l1_tft_spi_unlock(INT8U tft_spi_dev)
{
    TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);

#if TFT_SPI_SYSTEM_OS_ENABLE == 1
	if(pTFT_SPIDev->Dev_hw_sem)
        osSemaphoreRelease(pTFT_SPIDev->Dev_hw_sem);
#endif
}

INT32U drv_l1_tft_spi_sts_get(TFT_SPI_DEV_ID tft_spi_dev)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    return tft_spi_Reg->TFT_SPI_STS;
}

INT32S drv_l1_tft_spi_sts_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U state)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_STS = state;

    return 0;
}

INT32U drv_l1_tft_spi_inte_get(TFT_SPI_DEV_ID tft_spi_dev)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    return tft_spi_Reg->TFT_SPI_CTRL;
}

void drv_l1_tft_spi_write_command_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U set_4wire, INT32U command)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U state;

    drv_l1_tft_spi_clock_set(tft_spi_dev, TFT_SPI_CLOCK_ENABLE);

    state = 1;
    tft_spi_Reg->TFT_SPI_CTRL = (0x40001 | (state << B_TFT_SPI_CLOCK_OUT));

    state = 0;
    state |= (1<<3);
    if(set_4wire) // 4wire
        state |= 0x4001;
    else
        state |= 0x0001;
    tft_spi_Reg->TFT_SPI_CTRL2 = state;
    tft_spi_Reg->TFT_SPI_TX_DAT = command;

    while(1)
    {
        state = drv_l1_tft_spi_sts_get(tft_spi_dev);
        if(state & C_TFT_SPI_TX_DONE)
        {
            drv_l1_tft_spi_sts_set(tft_spi_dev, state);
            break;
        }
    }
}

void drv_l1_tft_spi_write_data_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U set_4wire, INT32U data)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U state;

    drv_l1_tft_spi_clock_set(tft_spi_dev, TFT_SPI_CLOCK_ENABLE);

    state = 1;
    tft_spi_Reg->TFT_SPI_CTRL = (0x40001 | (state << B_TFT_SPI_CLOCK_OUT));
    state = 0;
    state |= (1<<3);
    if(set_4wire) // 4wire
        state |= 0x6001;
    else
        state |= 0x2001;
    tft_spi_Reg->TFT_SPI_CTRL2 = state;
    tft_spi_Reg->TFT_SPI_TX_DAT = data;

    while(1)
    {
        state = drv_l1_tft_spi_sts_get(tft_spi_dev);
        if(state & C_TFT_SPI_TX_DONE)
        {
            drv_l1_tft_spi_sts_set(tft_spi_dev, state);
            break;
        }
    }
}


static void TFT_SPI2_IRQHandler(void)
{
    TFT_SPIDev_t *pTFT_SPIDev;
    INT32U msg,enable,status;

    pTFT_SPIDev = get_tft_spi_device(TFT_SPI1);

    status = drv_l1_tft_spi_sts_get(TFT_SPI1);
    drv_l1_tft_spi_sts_set(TFT_SPI1, status);
    enable = drv_l1_tft_spi_inte_get(TFT_SPI1);

        msg = 0;
        if((enable & C_TFT_SPI_TX_INTE_ENABLE) && (status & C_TFT_SPI_TX_DONE))
        {
            msg |= C_TFT_SPI_TX_DONE;
            if(display_isr_callback)
            display_isr_callback(TFT_SPI1, msg);
        }
        else
        {
                if(pTFT_SPIDev->device_update_flag)
                {
                    R_TV_FBI_ADDR = pTFT_SPIDev->device_frame_buf;
                    pTFT_SPIDev->device_update_flag = 0;
                    #if TFT_SPI_SYSTEM_OS_ENABLE == 1
                        osMessagePut(pTFT_SPIDev->message_queue, (uint32_t)&msg, osWaitForever);
                    #endif
                }
        }

        pTFT_SPIDev->interrupt_flag = msg;

        if(tft_spi_isr_callback)
        {
            tft_spi_isr_callback(TFT_SPI1, msg);
        }
}

/**
 * @brief   Initialize TFT spi controller
 * @param   none
 * @return 	none
 */
void drv_l1_tft_spi_init(TFT_SPI_DEV_ID tft_spi_dev)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);
    INT32U i;

    drv_l1_tft_spi_clock_set(tft_spi_dev, TFT_SPI_CLOCK_ENABLE);

    if(tft_spi_dev == TFT_SPI0)
    {
        R_SYSTEM_MISC_CTRL6 |= 0x20;
        for(i=0;i<10;i++);
        R_SYSTEM_MISC_CTRL6 &= ~0x20;
    }
    else
    {
        R_SYSTEM_MISC_CTRL6 |= 0x10;
        for(i=0;i<10;i++);
        R_SYSTEM_MISC_CTRL6 &= ~0x10;
    }

    tft_spi_Reg->TFT_SPI_CTRL = 0;
    i = 0;
    i |= (0x1 << 3);
    tft_spi_Reg->TFT_SPI_CTRL2 = i;
    tft_spi_Reg->TFT_SPI_CTRL3 = 0;

    if(tft_spi_dev_init_flag[tft_spi_dev] == 0)
    {
        gp_memset((INT8S *)pTFT_SPIDev, 0, sizeof(TFT_SPIDev_t));
        tft_spi_dev_init_flag[tft_spi_dev] = 1;
    }

	if (pTFT_SPIDev->init_done == 0) {
		pTFT_SPIDev->init_done = 1;
    #if TFT_SPI_SYSTEM_OS_ENABLE == 1
        if(pTFT_SPIDev->message_queue == NULL) {
            osMessageQDef_t tft_spi_q = { C_TFT_SPI_QUEUE_SIZE, sizeof(INT32U), 0 };

            pTFT_SPIDev->message_queue = osMessageCreate(&tft_spi_q, NULL);
            if(pTFT_SPIDev->message_queue == NULL)
               pTFT_SPIDev->init_done = 0;
        }

        if(pTFT_SPIDev->Dev_hw_sem == NULL) {
            osSemaphoreDef_t tft_spi_sem = { 0 };

            pTFT_SPIDev->Dev_hw_sem = osSemaphoreCreate(&tft_spi_sem, 1);
            if(pTFT_SPIDev->Dev_hw_sem == NULL)
               pTFT_SPIDev->init_done = 0;
        }
    #endif
    	if(tft_spi_dev == TFT_SPI0)
    	{
        	vic_irq_register(TFT_SPI1_IRQ_NUM, TFT_SPI1_IRQHandler);
        	vic_irq_enable(TFT_SPI1_IRQ_NUM);
    	#if TFT_SPI_FIQ_EN == 1
        	vic_fiq_enable(TFT_SPI1_IRQ_NUM);
    	#endif
    	}
    	else
    	{
        	vic_irq_register(TFT_SPI2_IRQ_NUM, TFT_SPI2_IRQHandler);
        	vic_irq_enable(TFT_SPI2_IRQ_NUM);
    	#if TFT_SPI_FIQ_EN == 1
        	vic_fiq_enable(TFT_SPI2_IRQ_NUM);
    	#endif
    	}
	}
}

void drv_l1_tft_spi_uninit(TFT_SPI_DEV_ID tft_spi_dev)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_CTRL = 0;
    tft_spi_Reg->TFT_SPI_CTRL2 = 0;
    tft_spi_Reg->TFT_SPI_CTRL3 = 0;

    if(pTFT_SPIDev->init_done)
    {
        vic_irq_disable(tft_spi_dev);
        drv_l1_tft_spi_clock_set(tft_spi_dev, TFT_SPI_CLOCK_DISABLE);
    }
}

INT32S drv_l1_tft_spi_ck_pha(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_CLOCK_PHASE_2DATA;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_CLOCK_PHASE_2DATA;

    return 0;
}

INT32S drv_l1_tft_spi_ck_pol(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_CLOCK_PHASE_HIGH;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_CLOCK_PHASE_HIGH;

    return 0;
}

INT32S drv_l1_tft_spi_manual_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_MEM_MODE;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_MEM_MODE;

    return 0;
}

INT32S drv_l1_tft_spi_tx_inte_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_TX_INTE_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_TX_INTE_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_colok_out_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U clock_out)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U value = tft_spi_Reg->TFT_SPI_CTRL;

    value &= ~C_MASK_TFT_SPI_CLOCK_OUT;
    value |= ((clock_out << B_TFT_SPI_CLOCK_OUT) & C_MASK_TFT_SPI_CLOCK_OUT);
    tft_spi_Reg->TFT_SPI_CTRL = value;

    return 0;
}

INT32S drv_l1_tft_spi_rx_data_sel_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U rx_data_sel)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U value = tft_spi_Reg->TFT_SPI_CTRL;

    value &= ~C_MASK_TFT_SPI_RX_DATA_SEL;
    value |= ((rx_data_sel << B_TFT_SPI_RX_DATA_SEL) & C_MASK_TFT_SPI_RX_DATA_SEL);
    tft_spi_Reg->TFT_SPI_CTRL = value;

    return 0;
}

INT32S drv_l1_tft_spi_tx_disable_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U disable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(disable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_TX_DISABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_TX_DISABLE;

    return 0;
}

INT32S drv_l1_tft_spi_master_rx_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL |= C_TFT_SPI_MASTER_RX_MODE;
    else
        tft_spi_Reg->TFT_SPI_CTRL &= ~C_TFT_SPI_MASTER_RX_MODE;

    return 0;
}
/*
This api is for Dual screen case , when switch screen , we
can according tft spi enable bit to decide if intinal LCD
*/
INT32U drv_l1_tft_spi_enable_status(TFT_SPI_DEV_ID tft_spi_dev)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    return (tft_spi_Reg->TFT_SPI_CTRL2 & C_TFT_SPI_ENABLE);
}

INT32S drv_l1_tft_spi_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_rgb_order_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_BGR_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_BGR_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_manual_frame_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U data_en)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(data_en)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_DATA_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_DATA_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_ppu_mem_continue_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_CONTINUE_FRAME_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_CONTINUE_FRAME_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_4wire_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_4WIRE_MODE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_4WIRE_MODE;

    return 0;
}

INT32S drv_l1_tft_spi_ppu_mem_start(TFT_SPI_DEV_ID tft_spi_dev, INT32U start)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U value = tft_spi_Reg->TFT_SPI_CTRL2;

    if(start)
        value |= (C_TFT_SPI_MEM_PPU_ENABLE | C_TFT_SPI_ENABLE);
    else
        value &= ~C_TFT_SPI_MEM_PPU_ENABLE;

    tft_spi_Reg->TFT_SPI_CTRL2 = value;

    return 0;
}

INT32S drv_l1_tft_spi_te_line_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U line_cnt, INT32U hsync_cnt)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);
    INT32U value = tft_spi_Reg->TFT_SPI_TE_CNT;

    value &= ~(C_MASK_TFT_TE_LINE_CNT | C_MASK_TFT_SPI_TE_HSYNC_CNT);
    value |= (((line_cnt << B_TFT_SPI_TE_LINE_CNT) & C_MASK_TFT_TE_LINE_CNT)
    | ((hsync_cnt << B_TFT_SPI_TE_HSYNC_CNT) & C_MASK_TFT_SPI_TE_HSYNC_CNT)) ;

    tft_spi_Reg->TFT_SPI_TE_CNT = value;

    return 0;
}

INT32S drv_l1_tft_spi_te_mode_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U mode)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(mode)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_TE_VSYNC_HSYNC_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_TE_VSYNC_HSYNC_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_te_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
        tft_spi_Reg->TFT_SPI_CTRL2 |= C_TFT_SPI_TE_ENABLE;
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_TE_ENABLE;

    return 0;
}

INT32S drv_l1_tft_spi_io_keep_enable(TFT_SPI_DEV_ID tft_spi_dev, INT32U enable)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(enable)
    {
        tft_spi_Reg->TFT_SPI_CTRL2 |= (C_TFT_SPI_IO_KEEP_ENABLE | C_TFT_SPI_ENABLE);
    }
    else
        tft_spi_Reg->TFT_SPI_CTRL2 &= ~C_TFT_SPI_IO_KEEP_ENABLE;

    return 0;
}


INT32S drv_l1_tft_spi_read_dummy_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U read_cnt, INT32U dummy_cnt)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_CTRL3 = (((dummy_cnt << B_TFT_SPI_DUMMY_CNT) & C_MASK_TFT_SPI_DUMMY_CNT)
    | ((read_cnt << B_TFT_SPI_READ_CNT) & C_MASK_TFT_SPI_READ_CNT));

    return 0;
}

INT32S drv_l1_tft_spi_vsync_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U vsync_width, INT32U vsync_back_porch, INT32U vsync_front_porch)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_VSYNC = (((vsync_front_porch << B_TFT_SPI_VSYNC_FRONT_PORCH) & C_MASK_TFT_SPI_VSYNC_FRONT_PORCH)
    | ((vsync_back_porch << B_TFT_SPI_VSYNC_BACK_PORCH) & C_MASK_TFT_SPI_VSYNC_BACK_PORCH)
    | ((vsync_width << B_TFT_SPI_VSYNC_WIDTH) & C_MASK_TFT_SPI_VSYNC_WIDTH));

    return 0;
}

INT32S drv_l1_tft_spi_v_disp_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U v_disp_width)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_VDISP = v_disp_width;//(v_disp_width*2);

    return 0;
}

INT32S drv_l1_tft_spi_hsync_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U hsync_width, INT32U hsync_back_porch, INT32U hsync_front_porch)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_HSYNC = (((hsync_front_porch << B_TFT_SPI_HSYNC_FRONT_PORCH) & C_MASK_TFT_SPI_HSYNC_FRONT_PORCH)
    | ((hsync_back_porch << B_TFT_SPI_HSYNC_BACK_PORCH) & C_MASK_TFT_SPI_HSYNC_BACK_PORCH)
    | ((hsync_width << B_TFT_SPI_HSYNC_WIDTH) & C_MASK_TFT_SPI_HSYNC_WIDTH));

    return 0;
}

INT32S drv_l1_tft_spi_h_disp_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U h_disp_width, INT32U wire_3_en)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    if(wire_3_en)
        tft_spi_Reg->TFT_SPI_HDISP = ((h_disp_width*9*2*2) & C_MASK_TFT_SPI_H_DISP);
    else
        tft_spi_Reg->TFT_SPI_HDISP = ((h_disp_width*8*2*2) & C_MASK_TFT_SPI_H_DISP);

    return 0;
}

INT32S drv_l1_tft_spi_data_cnt_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U h_disp_width, INT32U v_disp_width)
{
    TFT_SPI_DEVICE_SFR *tft_spi_Reg = get_tft_spi_dev_reg_base(tft_spi_dev);

    tft_spi_Reg->TFT_SPI_DATA_CNT = ((h_disp_width*v_disp_width*2) & C_MASK_TFT_SPI_DATA_CNT);


    return 0;
}

INT32S drv_l1_tft_spi_update_frame_wait(TFT_SPI_DEV_ID tft_spi_dev, INT32U wait)
{
    TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);

#if TFT_SPI_SYSTEM_OS_ENABLE == 1
    osEvent result;

    if(wait)
        result = osMessageGet(pTFT_SPIDev->message_queue, osWaitForever);
    else
        result = osMessageGet(pTFT_SPIDev->message_queue, 1);
    if (result.status != osEventMessage)
        return 0;
    else
        return result.value.v;
#else
    if(wait)
    {
        while(1)
        {
            if(pTFT_SPIDev->interrupt_flag & C_TFT_SPI_TX_DONE)
            {
                pTFT_SPIDev->interrupt_flag = 0;
                break;
            }
        }

        return C_TFT_SPI_TX_DONE;
    }
    else
        return pTFT_SPIDev->interrupt_flag;
#endif
}

INT32S drv_l1_tft_spi_update_frame_set(TFT_SPI_DEV_ID tft_spi_dev, INT32U frame, INT32U wait)
{
    TFT_SPIDev_t *pTFT_SPIDev = get_tft_spi_device(tft_spi_dev);

    pTFT_SPIDev->device_update_flag = 1;
    pTFT_SPIDev->device_frame_buf = frame;
    if(wait)
        drv_l1_tft_spi_update_frame_wait(tft_spi_dev, wait);

    return 0;
}

void drv_l1_tft_spi_isr_callback_set(void (*callback)(INT8U tft_spi_dev, INT32U tft_spi_event))
{
    tft_spi_isr_callback = callback;
}

void drv_l1_tft_spi_display_callback_set(void (*callback)(INT8U tft_spi_dev, INT32U tft_spi_event))
{
    display_isr_callback = callback;
}
