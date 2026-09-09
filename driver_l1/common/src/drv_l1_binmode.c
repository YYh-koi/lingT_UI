/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
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


static void BINMODE_A_IRQHandler(void);
static void BINMODE_B_IRQHandler(void);
static void (*binmode_A_isr_callback)(INT32U dev_no, INT32U event);
static void (*binmode_B_isr_callback[BINMODE_B_NUM])(INT32U dev_no, INT32U event);
/**
 * @brief   get binmode register base address
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return 	binmode register base address
 */
static binmodeReg_t* binmode_select(INT8U binmode_num)
{
	if(binmode_num == BINMODE_A) {
		return (binmodeReg_t*)P_BINMODE_A_BASE;
	}else if(binmode_num == BINMODE_B){
        return (binmodeReg_t*)P_BINMODE_B_BASE;
	}else if(binmode_num == BINMODE_B1){
        return (binmodeReg_t*)P_BINMODE_B1_BASE;
	}else if(binmode_num == BINMODE_B2){
        return (binmodeReg_t*)P_BINMODE_B2_BASE;
	}else if(binmode_num == BINMODE_B3){
        return (binmodeReg_t*)P_BINMODE_B3_BASE;
	}
	return 0;
}
/**
 * @brief   get binmode interrupt flag
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return 	interrupt flag
 */
INT32U drv_l1_binmode_int_flag_get(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    return pBinmodeReg->INT_FLAG;
}
/**
 * @brief   set binmode interrupt flag
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param   interrupt flag
 *  bit0:frame end
 *  bit4:bufA end
 *  bit5:bufB end
 * @return 	none
 */
INT32S drv_l1_binmode_int_flag_set(INT8U binmode_num, INT32U int_flag)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    pBinmodeReg->INT_FLAG |= int_flag;

    return 0;
}
/**
 * @brief   set binmode interrupt enable
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	int_en: interrupt enable
 * @return  none
 */
INT32S drv_l1_binmode_interrupt_set(INT8U binmode_num, INT32U int_en)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    pBinmodeReg->INT_FLAG = 0x10031; //clear interrupt flag
    pBinmodeReg->INT_EN = int_en; //set interrupt

    return 0;
}
/**
 * @brief   binmode interrupt register
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	enable, 1:enable
 * @param  	vic_num, 0:IRQ, 1~4:FIQ number
 * @param  	isr_handler: callback function
 * @return  none
 */
INT32S drv_l1_binmode_interrupt_register(INT8U binmode_num, void (*user_isr_callback)(INT32U dev_no, INT32U event))
{
    switch(binmode_num)
    {
        case BINMODE_A:
            binmode_A_isr_callback = user_isr_callback;
        break;
        case BINMODE_B:
        case BINMODE_B1:
        case BINMODE_B2:
        case BINMODE_B3:
            binmode_B_isr_callback[binmode_num-1] = user_isr_callback;
        break;
        default:return -1;break;
    }
    return 0;
}
/**
 * @brief   set binmode config
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	config
 * bit mode, 0:8bit, 1:10bit
 * by_m, 0:by2, 1:by4
 * mode, 0:avg, 1:skip, 2:bypass
 * crop_enable, 1:enable
 * crop_x, crop_y : crop start point
 * crop_height, crop_width : crop image size
 * sensor_height, sensor_width : input size (sensor output to binmode)
 * @return  none
 */
INT32S drv_l1_binmode_config(INT8U binmode_num, P_BINMODE_CDSP_CFG config)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    pBinmodeReg->CTRL &= ~0x1; //disable binmode
    //pBinmodeReg->SOFT_RST = 0x1; //rst binmode
    pBinmodeReg->CTRL = ((config->pixwidth10Pad & 0x1) << 24) | ((config->bit_mode & 0x1) << 14) \
                        | ((config->by_m & 0x1) << 8) | ((config->mode & 0x3) << 4) | ((config->crop_enable & 0x1) << 12);

    pBinmodeReg->CROP_START = ((config->crop_y & 0x1FFF) << 16) | (config->crop_x & 0x1FFF);
    pBinmodeReg->CROP_SIZE = ((config->crop_height & 0x1FFF) << 16) | (config->crop_width & 0x1FFF);
    pBinmodeReg->IN_SIZE = ((config->sensor_height & 0x1FFF) << 16) | (config->sensor_width & 0x1FFF);
    //binmode output size (binmode_B only)
    if((config->mode & 0x3) == 0x2)//bypass mode
        pBinmodeReg->OUT_SIZE = config->sensor_height * config->sensor_width;
    else
        if((config->by_m & 0x1) == 0x0)//by2
            pBinmodeReg->OUT_SIZE = config->sensor_height * config->sensor_width / 4;
        else
            pBinmodeReg->OUT_SIZE = config->sensor_height * config->sensor_width / 16;

    return 0;
}
/**
 * @brief   binmode system FIFO update flag set
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	enable, 1:enable
 * @return  none
 */
INT32S drv_l1_binmode_sysfifo_set(INT8U binmode_num, INT8U enable)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    if(enable)
        pBinmodeReg->CTRL |= (0x1<<16);
    else
        pBinmodeReg->CTRL &= ~(0x1<<16);

    return 0;
}
/**
 * @brief   binmode frame buffer A set
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	buffer: buffer address
 * @return  none
 */
void drv_l1_binmode_framebuf_A_set(INT8U binmode_num, INT32U buffer)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    pBinmodeReg->FRAME_A_BUF = buffer;
}
/**
 * @brief   binmode frame buffer B set
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @param  	buffer: buffer address
 * @return  none
 */
void drv_l1_binmode_framebuf_B_set(INT8U binmode_num, INT32U buffer)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    pBinmodeReg->FRAME_B_BUF = buffer;
}
/**
 * @brief   binmode frame buffer A get
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return  buffer: buffer address
 */
INT32U drv_l1_binmode_framebuf_A_get(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    return pBinmodeReg->FRAME_A_BUF;
}
/**
 * @brief   binmode frame buffer B get
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return  buffer: buffer address
 */
INT32U drv_l1_binmode_framebuf_B_get(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);

    return pBinmodeReg->FRAME_B_BUF;
}
/**
 * @brief   binmode output width get
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return  output width
 */
INT32U drv_l1_binmode_get_cfg_out_width(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    if(pBinmodeReg->CTRL & 0x00001000)//crop
    {
        if((pBinmodeReg->CTRL & 0x00000030) == 0x00000020)//bypass
        {
            return pBinmodeReg->CROP_SIZE & 0x1FFF;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000000)//by2
        {
            return (pBinmodeReg->CROP_SIZE & 0x1FFF)>>1;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000100)//by4
        {
            return (pBinmodeReg->CROP_SIZE & 0x1FFF)>>2;
        }
    }
    else
    {
        if((pBinmodeReg->CTRL & 0x00000030) == 0x00000020)//bypass
        {
            return pBinmodeReg->IN_SIZE & 0x1FFF;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000000)//by2
        {
            return (pBinmodeReg->IN_SIZE & 0x1FFF)>>1;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000100)//by4
        {
            return (pBinmodeReg->IN_SIZE & 0x1FFF)>>2;
        }
    }
    return 0;
}
/**
 * @brief   binmode output height get
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return  output height
 */
INT32U drv_l1_binmode_get_cfg_out_height(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    if(pBinmodeReg->CTRL & 0x00001000)//crop
    {
        if((pBinmodeReg->CTRL & 0x00000030) == 0x00000020)//bypass
        {
            return (pBinmodeReg->CROP_SIZE & 0x1FFF0000)>>16;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000000)//by2
        {
            return (pBinmodeReg->CROP_SIZE & 0x1FFF0000)>>17;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000100)//by4
        {
            return (pBinmodeReg->CROP_SIZE & 0x1FFF0000)>>18;
        }
    }
    else
    {
        if((pBinmodeReg->CTRL & 0x00000030) == 0x00000020)//bypass
        {
            return (pBinmodeReg->IN_SIZE & 0x1FFF0000)>>16;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000000)//by2
        {
            return (pBinmodeReg->IN_SIZE & 0x1FFF0000)>>17;
        }
        else if((pBinmodeReg->CTRL & 0x00000300) == 0x00000100)//by4
        {
            return (pBinmodeReg->IN_SIZE & 0x1FFF0000)>>18;
        }
    }
    return 0;
}
/**
 * @brief   binmode disable
 * @param   binmode_num, 0:binmode1, 1:binmode2
 * @return  none
 */
INT32S drv_l1_binmode_disable(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    pBinmodeReg->CTRL &= ~(0x1<<0);
    pBinmodeReg->INT_FLAG = 0x10031; //clear interrupt flag
    return 0;
}

INT32S drv_l1_binmode_enable(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    pBinmodeReg->INT_FLAG = 0x10031; //clear interrupt flag
    pBinmodeReg->CTRL |= (0x1<<0);
    return 0;
}

INT32S drv_l1_binmode_set_ctrl(INT8U binmode_num, INT32U ctrl)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    pBinmodeReg->CTRL = ctrl;
    return 0;
}

INT32S drv_l1_binmode_init(INT8U binmode_num, INT8U vic_num)
{
    if(binmode_num == BINMODE_A) {
        drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_A, ENABLE);
        vic_irq_register(VIC_BINMODE, BINMODE_A_IRQHandler);
		vic_irq_enable(VIC_BINMODE);
        if (vic_num) {
            vic_fiq_enable(VIC_BINMODE);
        } else {
			vic_fiq_disable(VIC_BINMODE);
        }
    } else {
        drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_B, ENABLE);
        vic_irq_register(VIC_BINMODE_B, BINMODE_B_IRQHandler);
		vic_irq_enable(VIC_BINMODE_B);
        if (vic_num) {
			vic_fiq_enable(VIC_BINMODE_B);
        } else {
            vic_fiq_disable(VIC_BINMODE_B);
        }
    }
    return 0;
}

INT32S drv_l1_binmode_uninit(INT8U binmode_num, INT8U vic_num)
{
    if(binmode_num == BINMODE_A)
    {
        drv_l1_clock_set_system_clk_en(CLK_EN7_MIPI_RX_A, DISABLE);
        vic_irq_disable(VIC_BINMODE);
        vic_fiq_disable(VIC_BINMODE);
    } else {
        //drv_l1_clock_set_system_clk_en(CLK_EN2_MIPI_RX_B, DISABLE);
        vic_irq_disable(VIC_BINMODE_B);
        vic_fiq_disable(VIC_BINMODE_B);
    }
    return 0;
}

INT32U drv_l1_binmode_get_ctrl(INT8U binmode_num)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    return pBinmodeReg->CTRL;
}

static void BINMODE_A_IRQHandler(void)
{
	INT32U IRQ_Status, IRQ_Enable;
	binmodeReg_t *pBinmodeReg = binmode_select(BINMODE_A);

    IRQ_Status = pBinmodeReg->INT_FLAG;
    IRQ_Enable = pBinmodeReg->INT_EN;

    if(IRQ_Status) {
        // clear interrupt
        pBinmodeReg->INT_FLAG = IRQ_Status;

        IRQ_Status &= IRQ_Enable;
        if(binmode_A_isr_callback && IRQ_Status) {
            binmode_A_isr_callback(BINMODE_A, IRQ_Status);
        }
    }
}

static void BINMODE_B_IRQHandler(void)
{
	INT32U dev_no;
	INT32U IRQ_Status, IRQ_Enable;
	binmodeReg_t *pBinmodeReg;

	for(dev_no=BINMODE_B; dev_no<BINMODE_MAX_NUMS; dev_no++) {
		pBinmodeReg = binmode_select(dev_no);

		IRQ_Status = pBinmodeReg->INT_FLAG;
		IRQ_Enable = pBinmodeReg->INT_EN;

		if(IRQ_Status) {
			// clear interrupt
			pBinmodeReg->INT_FLAG = IRQ_Status;

			IRQ_Status &= IRQ_Enable;
			if(binmode_B_isr_callback[dev_no-1] && IRQ_Status) {
				binmode_B_isr_callback[dev_no-1](dev_no, IRQ_Status);
			}
		}
	}
}

void drv_l1_binmode_set_4pixel_mode(INT8U binmode_num, INT8U enable)
{
    binmodeReg_t *pBinmodeReg = binmode_select(binmode_num);
    if(enable)
        pBinmodeReg->CTRL |= 0x1<<20;
    else
        pBinmodeReg->CTRL &= ~(0x1<<20);
}
